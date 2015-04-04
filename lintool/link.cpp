/*
 * Copyright (c) 2012-2015, Mike Smith, <msmith@purgatory.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * o Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "exceptions.h"
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <err.h>
#include <sys/time.h>

#include "link.h"
#include "log.h"

namespace Link
{
libusb_context              *_usb_ctx = nullptr;
struct libusb_device_handle *_usb_handle = nullptr;
struct timeval              master_timeout;

constexpr uint16_t  bytes_to_short(uint8_t low, uint8_t high) { return ((uint16_t)high << 8) + low; }

void
connect()
{
    libusb_init(&_usb_ctx);

    // discover devices
    libusb_device **list = nullptr;
    ssize_t cnt = libusb_get_device_list(NULL, &list);
    ssize_t i = 0;
    int err = 0;

    if (cnt < 0) {
        RAISE(ExUSBFailed, "no USB devices found");
    }

    for (i = 0; i < cnt; i++) {
        libusb_device *device = list[i];
        struct libusb_device_descriptor desc;

        if (libusb_get_device_descriptor(device, &desc)) {
            RAISE(ExUSBFailed, "can't get device descriptor");
        }

        // match on vendor/device ID for now
        if ((desc.idVendor == bytes_to_short(USB_CFG_VENDOR_ID)) &&
            (desc.idProduct == bytes_to_short(USB_CFG_DEVICE_ID))) {

            err = libusb_open(device, &_usb_handle);

            if (err) {
                RAISE(ExUSBFailed, "open failed");
            }

            return;
        }
    }

    RAISE(ExConnectFailed, "LIN interface not found");
}


int
request(uint8_t bRequest, uint16_t wValue, uint16_t wIndex)
{
    return libusb_control_transfer(_usb_handle,
                                   LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN,
                                   bRequest,
                                   wValue,
                                   wIndex,
                                   nullptr,
                                   0,
                                   5000);
}

int
request_in(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength)
{
    return libusb_control_transfer(_usb_handle,
                                   LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN,
                                   bRequest,
                                   wValue,
                                   wIndex,
                                   data,
                                   wLength,
                                   5000);
}

int
request_out(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength)
{
    return libusb_control_transfer(_usb_handle,
                                   LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT,
                                   bRequest,
                                   wValue,
                                   wIndex,
                                   data,
                                   wLength,
                                   5000);
}

void
enable_master(bool enable)
{
    LIN_LOG_BLOCK;

    int result = request(kUSBRequestEnableMaster, enable ? 1 : 0, 0);

    if (result < 0) {
        warnx("master failed");
        RAISE(ExUSBFailed, "master: USB error " << result);
    }

    if (enable) {
        for (unsigned tries = 0; tries < 20; tries++) {
            usleep(50000);

            // check for bus no longer awake
            if (!(get_status() & RQ_STATUS_AWAKE)) {
                return;
            }
        }

        RAISE(ExLinkFailed, "cannot claim the bus");

    } else {
        timerclear(&master_timeout);
    }
}

void
refresh_master()
{
    struct timeval now;
    gettimeofday(&now, nullptr);

    if (timercmp(&now, &master_timeout, >)) {
        // poke the link back into master mode
        enable_master(true);
    }

    // The link drops master mode after ~1s of inactivity; if we have been idle for
    // more than half that, we want to re-enable it...
    struct timeval delta = {0, 500000U};
    timeradd(&now, &delta, &master_timeout);
}

uint8_t
get_status(unsigned which, unsigned index)
{
    uint8_t status = 0;

    int result = request_in(kUSBRequestStatus, index, which, &status, sizeof(status));

    if (result < 0) {
        RAISE(ExUSBFailed, "status: USB error " << result);
    }

    return status;
}

void
set_node(uint8_t node)
{
    int result = request(kUSBRequestSelectNode, node, 0);

    if (result < 0) {
        RAISE(ExUSBFailed, "set node: USB error " << result);
    }
}

void
write_param(uint16_t index, uint16_t value)
{
    LIN_LOG_BLOCK;
    refresh_master();

    int result = request(kUSBRequestWriteData, value, index);

    if (result < 0) {
        RAISE(ExUSBFailed, "write: USB error " << result);
    }
}

void
bulk_data(uint8_t *bytes)
{
    LIN_LOG_BLOCK;
    refresh_master();

    uint16_t value, index;

    value = bytes[0] + (((uint16_t)bytes[1]) << 8);
    index = bytes[2] + (((uint16_t)bytes[3]) << 8);

    int result = request(kUSBRequestSendBulk, value, index);

    if (result < 0) {
        RAISE(ExUSBFailed, "bulk: USB error " << result);
    }

}

uint16_t
read_param(uint16_t index)
{
    LIN_LOG_BLOCK;
    refresh_master();

    int result;
    uint16_t value;
    unsigned tries = 0;
    uint8_t status;

    for (;;) {
        result = request(kUSBRequestReadData, 0, index);

        if (result < 0) {
            RAISE(ExUSBFailed, "read1: USB error " << result);
        }

        status = get_status();

        if (status & RQ_STATUS_DATA_ERROR) {
            if (tries++ < 3) {
                continue;
            }

            RAISE(ExLINError, "persistent error in LIN data phase, status " << unsigned(status));
        }

        break;
    }

    if (status & RQ_STATUS_DATA_REJECTED) {
        RAISE(ExNoParam, "node rejected parameter");
    }

    if (status & RQ_STATUS_DATA_READY) {
        result = request_in(kUSBRequestReadResult, 0, 0, (uint8_t *)&value, sizeof(value));

        if (result < 0) {
            RAISE(ExUSBFailed, "read2: USB error " << result);
        }

        if (result != 2) {
            RAISE(ExLinkFailed, "bad data size in read: " << result);
        }

        return value;
    }

    RAISE(ExLINError, "transfer status error, status " << unsigned(status));

}

void
print_status()
{
    std::cerr << "STATUS: free memory:            "
              << (unsigned)Link::get_status(RQ_STATUS_FREEMEM)
              << std::endl;
    unsigned status = Link::get_status();
    std::cerr << "STATUS: status byte:            "
              << std::hex << std::setw(2) << std::setfill('0') << status
              << ((status & RQ_STATUS_DATA_READY) ? " DATA_READY" : "")
              << ((status & RQ_STATUS_DATA_ERROR) ? " DATA_ERROR" : "")
              << ((status & RQ_STATUS_AWAKE) ? " AWAKE" : "")
              << ((status & RQ_STATUS_WAITING) ? " WAITING" : "")
              << ((status & RQ_STATUS_MASTER) ? " MASTER" : "")
              << std::endl;
    std::cerr << "STATUS: line errors:            " << (unsigned)Link::get_status(RQ_STATUS_LINERR, 0) << std::endl;
    std::cerr << "STATUS: checksum errors:        " << (unsigned)Link::get_status(RQ_STATUS_LINERR, 1) << std::endl;
    std::cerr << "STATUS: parity errors:          " << (unsigned)Link::get_status(RQ_STATUS_LINERR, 2) << std::endl;
    std::cerr << "STATUS: framing errors:         " << (unsigned)Link::get_status(RQ_STATUS_LINERR, 3) << std::endl;
    std::cerr << "STATUS: synchronisation errors: " << (unsigned)Link::get_status(RQ_STATUS_LINERR, 4) << std::endl;
    std::cerr << "STATUS: protocol errors:        " << (unsigned)Link::get_status(RQ_STATUS_LINERR, 5) << std::endl;
}

} // namespace Link

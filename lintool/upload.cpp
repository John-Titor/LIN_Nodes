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

#include <unistd.h>
#include <err.h>
#include <iostream>
#include <iomanip>

#include "upload.h"
#include "link.h"
#include "log.h"

#include <lin_defs.h>

namespace Upload
{

unsigned pagesize;

uint8_t
lo8(uint16_t val)
{
    return val & 0xff;
}

uint8_t
hi8(uint16_t val)
{
    return val >> 8;
}

uint16_t
crc_ccitt_update(uint16_t crc, uint8_t data)
{
    data ^= lo8(crc);
    data ^= data << 4;
    return ((((uint16_t)data << 8) | hi8(crc)) ^ (uint8_t)(data >> 4) ^ ((uint16_t)data << 3));
}

unsigned
page_crc(uint8_t *bytes, uint16_t address)
{
    uint16_t crc = 0xffff;
    unsigned count = pagesize;

    crc = crc_ccitt_update(crc, address & 0xff);
    crc = crc_ccitt_update(crc, address >> 8);

    while (count--) {
        crc = crc_ccitt_update(crc, *bytes++);
    }

    return crc;
}

unsigned
status()
{
    // this should block until the link can get a value from the bootloader
    try {
        return Link::read_param(Bootloader::kParamStatus);

    } catch (Exception &e) {
        RAISE(ExLink, "error reading bootloader status: " << e.what());
    }
}

void
check_ready()
{
    auto s = status();

    if ((s == bl_status::kWaitingForProgrammer) ||
        (s == bl_status::kReadyForPage)) {
        return;
    }

    // XXX we can remove this, check at end of page should catch it
    if (s == bl_status::kPageAddressError) {
        RAISE(ExBadAddress, "bootloader not ready: address error");
    }

    if (s == bl_status::kPageCRCError) {
        RAISE(ExCRCError, "bootloader not ready: CRC error");
    }

    if (s == bl_status::kPageInProgress) {
        RAISE(ExProtocol, "bootloader not ready: page still/already in progress");
    }

    RAISE(ExProtocol, "bootloader not ready, unexpected status: " << s);
}

void
reset_bootloader()
{
    if (Link::read_param(Generic::kParamOperationMode) != operation_magic::kBootloader) {
        RAISE(ExProtocol, "not in bootloader mode");
    }

    Link::write_param(Generic::kParamOperationMode, 1);
}

void
set_address(unsigned address)
{
    Link::write_param(Bootloader::kParamPageAddress, address);

    // XXX we can remove this, check at end of page should catch it
    if (status() == bl_status::kPageAddressError) {
        RAISE(ExBadAddress, "bootloader rejected firmware due to address error");
    }
}

unsigned
read_bl_memory(unsigned address)
{
    for (auto tries = 0; tries < 3; tries++) {
        auto value = Link::read_param(Bootloader::kParamMemory);
        auto set_address = Link::read_param(Bootloader::kParamDebugPointer);

        if (set_address == (address + 1)) {
            return value;
        }

        Link::write_param(Bootloader::kParamDebugPointer, address);
    }

    RAISE(ExBadAddress, "can't set bootloader readback address");
}

unsigned
read_bl_eeprom(unsigned address)
{
    Link::write_param(Bootloader::kParamDebugPointer, address);
    return Link::read_param(Bootloader::kParamEEPROM);
}

void
program_page(unsigned address, uint8_t *bytes, bool readback)
{
    unsigned crc = page_crc(bytes, address);

    check_ready();

    for (auto tries = 0; tries < 3; tries++) {

        std::cerr << "STATUS: program: 0x"
                  << std::hex << std::setw(4) << address;

        if (tries > 0) {
            std::cerr << " (retry)";
        }

        std::cerr << std::endl;

        // set the page address
        set_address(address);

        // upload data to the page buffer
        for (unsigned offset = 0; offset < pagesize; offset += 4) {
            Link::bulk_data(bytes + offset);
        }

        // send our CRC to the bootloader; it will either program the
        // page or it will flag a CRC error
        Link::write_param(Bootloader::kParamPageCRC, crc);

        // wait for the page to be programmed
        try {
            check_ready();

        } catch (ExCRCError &e) {
            // we are prepared to retry here...
            std::cerr << "WARNING: CRC error ("
                      << tries
                      << ") we have "
                      << crc
                      << " bootloader wanted "
                      << Link::read_param(Bootloader::kParamPageCRC)
                      << std::endl;
            reset_bootloader();
            continue;

        } catch (ExProtocol &e) {
            // write may have been dropped - page still thinks it wants more data
            std::cerr << "WARNING: page write failed ("
                      << tries
                      << ")"
                      << std::endl;
            continue;

        } catch (ExBadAddress &e) {
            // address write may have been corrupted
            std::cerr << "WARNING: page address failed ("
                      << tries
                      << ")"
                      << std::endl;
            continue;
        }

        if (readback) {
            std::cerr << "STATUS: verify 0x"
                      << std::hex << std::setfill('0') << std::setw(4) << address
                      << std::endl;

            for (unsigned offset = 0; offset < pagesize; offset++) {
                // readback for reset vector will never compare, so ignore it
                if ((address + offset) <= 3) {
                    continue;
                }

                for (unsigned readback_tries = 0;; readback_tries++) {

                    if (read_bl_memory(address + offset) == bytes[offset]) {
                        break;
                    }

                    if (readback_tries > 3) {
                        RAISE(ExVerifyError, "address " << address + offset << " persistent readback miscompare");
                    }
                }
            }
        }

        return;
    }

    RAISE(ExLink, "too many upload CRC errors");
}

void
dump_page(unsigned address, unsigned size)
{
    for (auto offset = 0U; offset < size; offset += 8) {
        warnx("%04x: %02x %02x %02x %02x %02x %02x %02x %02x",
              address + offset,
              read_bl_memory(address + offset + 0),
              read_bl_memory(address + offset + 1),
              read_bl_memory(address + offset + 2),
              read_bl_memory(address + offset + 3),
              read_bl_memory(address + offset + 4),
              read_bl_memory(address + offset + 5),
              read_bl_memory(address + offset + 6),
              read_bl_memory(address + offset + 7));
    }
}

void
dump_eeprom(unsigned address, unsigned size)
{
    for (auto offset = 0U; offset < size; offset += 4) {
        warnx("%03x: %02x %02x %02x %02x",
              address + offset,
              read_bl_eeprom(address + offset + 0),
              read_bl_eeprom(address + offset + 1),
              read_bl_eeprom(address + offset + 2),
              read_bl_eeprom(address + offset + 3));
    }
}

void
upload(Firmware *fw, bool readback)
{
    pagesize = Link::read_param(Generic::kParamFirmwarePageSize);

    reset_bootloader();

    std::cerr << "STATUS: bl_reason: "
              << Encoding::info(kEncoding_bl_reason, Link::read_param(Bootloader::kParamReason))
              << std::endl;

    if (status() != bl_status::kWaitingForProgrammer) {
        RAISE(ExProtocol, "bootloader in unexpected state");
    }

    // loop writing pages
    unsigned address = pagesize;
    uint8_t bytes[pagesize];

    while (address < fw->max_address()) {

        if (fw->get_bytes(address, pagesize, &bytes[0])) {
            program_page(address, &bytes[0], readback);
        }

        address += pagesize;
    }

    if (fw->get_bytes(0, pagesize, &bytes[0])) {
        program_page(0, &bytes[0], readback);
    }
}

} // namespace Upload

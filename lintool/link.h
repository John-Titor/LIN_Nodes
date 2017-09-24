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
#pragma once

#include <stdint.h>
#include <stdexcept>
#include <libusb.h>

#include <usb_link/usbconfig.h>
#include <usb_link/requests.h>

#include "exceptions.h"


namespace Link
{

EXCEPTION(Exception, ExConnectFailed);
EXCEPTION(Exception, ExUSBFailed);
EXCEPTION(Exception, ExLinkFailed);
EXCEPTION(Exception, ExLINError);
EXCEPTION(Exception, ExNoParam);
EXCEPTION(Exception, ExUsageError);

// connect to the USB link
extern void     connect();

// low-level request methods
extern int      request(uint8_t bRequest, uint16_t wValue, uint16_t wIndex);
extern int      request_in(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength);
extern int      request_out(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength);

// link primitives
extern void     enable_master(bool enable);
extern uint8_t  get_status(unsigned which = RQ_STATUS_FLAGS, unsigned index = 0);
extern void     set_node(uint8_t node);
extern void     write_param(uint16_t index, uint16_t value);
extern uint16_t read_param(uint16_t index);
extern void     bulk_data(uint8_t *bytes);

// diagnostics
extern void     print_status();

} // namespace Link

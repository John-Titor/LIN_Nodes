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

#include <string.h>

#include "slave.h"
#include "board.h"

RelaySlave::RelaySlave(uint8_t nad) :
    Slave(nad)
{
}

void
RelaySlave::st_header_received()
{
    switch (current_FrameID()) {
    case kFrameIDRelays:
        st_expect_response();
        break;

    default:
        Slave::st_header_received();
        break;
    }
}

void
RelaySlave::st_response_received(Response &frame)
{
    switch (current_FrameID()) {

    case kFrameIDRelays:
        _relayFrame = frame;
        break;

    default:
        Slave::st_response_received(frame);
        break;
    }
}

bool
RelaySlave::st_read_data(Parameter::Address address, uint16_t &value)
{
    // Handle node parameters
    if (PowerV3::param_exists(address)) {
        value = Parameter(address).get();
        return true;
    }

    // pass to superclass
    return Slave::st_read_data(address, value);
}

bool
RelaySlave::st_write_data(Parameter::Address address, uint16_t value)
{
    uint8_t encoding = PowerV3::param_encoding(address);

    if ((encoding != kEncoding_none) && !Encoding::invalid(encoding, value)) {
        Parameter(address).set(value);
        return true;
    }

    return Slave::st_write_data(address, value);
}

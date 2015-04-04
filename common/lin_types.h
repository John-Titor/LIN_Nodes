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

class Parameter
{
public:
    typedef uint16_t Address;

    static const Address    noAddress = 0xffff;
    static const Address    configBase = 0x400;     // base of non-volatile config parameters
    static const Address    configTop = 0x4ef;
    static const Address    defaultBase = 0x500;    // base of read-only defaults for the 0x400-4ef range
    static const Address    defaultTop = 0x5ef;

    constexpr Parameter(Address address = noAddress) :
        _address(address)
    {
    }

    Parameter &operator = (const Parameter &p) { _address = p.address(); return *this; }

    //operator uint16_t () const { return get(); }
    //const Parameter & operator = (uint16_t value) const { set(value); return *this; }

    void            set(uint16_t value) const;  // must be implemented by the device
    uint16_t        get() const;                // must be implemented by the device

    constexpr uint16_t address() const { return _address; }
    constexpr bool  exists() const { return _address != noAddress; }

private:
    Address         _address;
};

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

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <iostream>

#include <lin_defs.h>

#include "firmware.h"


std::list<Firmware *>   Firmware::_firmwares;

Firmware::Firmware(const char *fromFile)
{
    char functionName[64];
    char revision[64];

    FILE *fp = fopen(fromFile, "r");

    if (fp == nullptr) {
        RAISE(ExIOFailed, "cannot open firmware file '" << fromFile << "'");
    }

    if (fscanf(fp, "Node: %64s %64s\n", functionName, revision) != 2) {
        RAISE(ExIOFailed, "invalid firmware file '" << fromFile << "'");
    }

    _functionName = strdup(functionName);

    // read 16-bit address intelhex
    while (!feof(fp)) {
        char line[100];

        line[0] = '\0';
        fgets(line, sizeof(line), fp);

        unsigned count;
        unsigned address;
        unsigned type;

        if (sscanf(line, ":%2x%4x%2x", &count, &address, &type) != 3) {
            continue;
        }

        unsigned sum = count + (address >> 8) + (address & 0xff) + type;
        unsigned pos = 9;

        if (type == 1) {
            break;
        }

        if (type == 0) {
            unsigned byte;

            while (count--) {
                if (sscanf(line + pos, "%2x", &byte) != 1) {
                    RAISE(ExIOFailed, "malformed firmware file '" << fromFile << "'");
                }

                sum += byte;
                _bytes[address] = byte;
                address++;
                pos += 2;
            }

            if (sscanf(line + pos, "%2x", &byte) != 1) {
                RAISE(ExIOFailed, "malformed firmware file '" << fromFile << "'");
            }

            sum = (~sum + 1) & 0xff;

            if (sum != byte) {
                RAISE(ExIOFailed, "checksum error in firmware file '" << fromFile << "'");
            }
        }
    }

    if (_bytes.empty()) {
        RAISE(ExIOFailed, "no data in firmware file '" << fromFile << "'");
    }

    // add ourselves to the list
    _firmwares.push_back(this);
    std::cerr << "STATUS: loaded "
              << functionName
              << " from "
              << fromFile
              << std::endl;
}

Firmware::~Firmware()
{
    if (_functionName) {
        delete _functionName;
    }
}

Firmware *
Firmware::for_function(unsigned function)
{
    if (_firmwares.size() == 0) {
        RAISE(ExLookup, "no firmware file loaded");
    }

    const char *name = Encoding::info(kEncoding_board_function, function);

    for (auto p : _firmwares) {
        if (!strcasecmp(name, p->_functionName)) {
            return p;
        }
    }

    return nullptr;
}

Firmware *
Firmware::implied_firmware()
{
    if (_firmwares.size() == 0) {
        RAISE(ExLookup, "no firmware file loaded");
    }

    if (_firmwares.size() == 1) {
        return _firmwares.front();
    }

    RAISE(ExLookup, "too many firmware files loaded");
}

bool
Firmware::get_bytes(unsigned base, unsigned count, uint8_t *buf) const
{
    bool result = false;

    while (count--) {
        auto it = _bytes.find(base);

        if (it == _bytes.end()) {
            *buf = 0xff;

        } else {
            *buf = it->second;
            result = true;
        }

        base++;
        buf++;
    }

    return result;
}

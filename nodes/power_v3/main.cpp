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

#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include "board.h"
#include "slave.h"
#include "mc17xsf500.h"

#include "lin_defs.h"

static uint8_t node_status;
static uint8_t output_status[MC17XSF500::num_channels];
static uint16_t current[MC17XSF500::num_channels];
static uint16_t info[8];
static uint16_t free_memory;

using namespace PowerV3;

static void
param_init()
{
    uint8_t options = eeprom_read_byte((uint8_t *)Board::kConfigOptions);
    bool should_default = (options & Board::kOptResetConfig);

    // init parameters (set to defaults if requested or not valid
    for (Parameter::Address addr = Parameter::configBase; addr < Parameter::configTop; addr++) {
        Parameter p(addr);
        uint8_t encoding = param_encoding(addr);

        if (should_default || ((encoding != kEncoding_none) && Encoding::invalid(encoding, p.get()))) {
            p.set(param_default(addr));
        }
    }

    if (should_default) {
        eeprom_update_byte((uint8_t *)Board::kConfigOptions, (options & ~Board::kOptResetConfig));
    }
}

uint8_t
get_assignment(uint8_t output)
{
    const uint16_t assign_base = kParamCH1Assign;
    const uint16_t channel_stride = kParamCH2Assign - kParamCH1Assign;

    return Parameter(assign_base + output * channel_stride).get();
}

void
main(void)
{
    Board::early_init();
    uint8_t     id = Board::get_mode();

    // check for recovery mode before constructing anything else
    if (id == 0) {
        Board::panic(Board::kPanicCodeRecovery);
    }

    // construct the slave
    param_init();
    RelaySlave slave(id);
    slave.init();

    // initialise the power switch
    MC17XSF500::configure();

    // enable interrupts
    sei();

    // XXX set up ADC trigger & collection here

    // run the output logic forever
    for (;;) {
        wdt_reset();
        slave.tick();

        // update free memory value each time it's read
        if (free_memory == 0) {
            free_memory = Board::freemem();
        }

        // update outputs - always do this to ensure that any lost update is fixed on
        // the next loop iteration
        for (unsigned output = 0; output < MC17XSF500::num_channels; output++) {

            uint8_t assigned = get_assignment(output);

            if ((assigned != v3_output_assignment::kUnassigned) && slave.test_relay(assigned)) {
                MC17XSF500::set(output, 1);

            } else {
                MC17XSF500::set(output, 0);
            }

            if (!assigned) {
                output_status[output] = v3_output_status::kUnassigned;

            } else {
                // XXX need to use time & knowledge of output type here to be more
                //     intelligent about the state of the load...

                auto status = MC17XSF500::get_status(MC17XSF500::kStatusCH1 + output);

                if (status.chX_status.ots != 0) {
                    output_status[output] = v3_output_status::kTemperatureShutdown;

                } else if (status.chX_status.otw != 0) {
                    output_status[output] = v3_output_status::kTemperatureWarning;

                } else if (status.chX_status.oc_status != 0) {
                    output_status[output] = v3_output_status::kOverCurrent;

                } else if ((status.chX_status.olon != 0) || (status.chX_status.oloff != 0)) {
                    output_status[output] = v3_output_status::kOpenLoad;

                } else {
                    output_status[output] = v3_output_status::kOK;
                }
            }
        }

        // check device status
        auto quick_status = MC17XSF500::get_status(MC17XSF500::kStatusQuick);
        auto device_status = MC17XSF500::get_status(MC17XSF500::kStatusDevice);
        node_status = v3_device_status::kOK;

        info[0] = quick_status.val;
        info[1] = device_status.val;
        info[2] = MC17XSF500::get_status(MC17XSF500::kStatusIO).val;

        // check overload, undervoltage, overvoltage
        if (quick_status.quick_status.ovlf != 0) {
            // prioritise output overload reporting
            node_status = v3_device_status::kOverload;

        } else if (device_status.device_status.uvf != 0) {
            node_status = v3_device_status::kUndervoltage;

        } else if (device_status.device_status.ovf != 0) {
            node_status = v3_device_status::kOvervoltage;
        }
    }
}

void
Parameter::set(uint16_t value) const
{
    switch (address()) {
    case Generic::kParamOperationMode:
        switch (value) {
        case operation_magic::kEnterBootloader:
            Board::enter_bootloader(RelaySlave::node_address(Board::get_mode()),
                                    board_function::kPowerV3);

        case operation_magic::kSetDefaults:
            Board::reset_to_defaults();
        }

        break;

    case Parameter::configBase ... Parameter::configTop:
        eeprom_update_word((uint16_t *)((address() - Parameter::configBase) * 2), value);
        break;
    }
}

uint16_t
Parameter::get() const
{
    switch (address()) {
    case Generic::kParamProtocolVersion:
        return Master::kNodeProtocolVersion;

    case Generic::kParamBoardFunction:
        return board_function::kPowerV3;

    case Generic::kParamOperationMode:
        return 0;

    case Generic::kParamFirmwareVersion:
        return RELEASE_VERSION;

    case Generic::kParamFirmwarePageSize:
        return SPM_PAGESIZE;

    case Generic::kParamFreeMem: {
        uint16_t temp = free_memory;
        free_memory = 0;
        return temp;
    }

    case Generic::kParamWatchdogResets:
        return Board::wdt_reset_count;

    case kParamDeviceStatus:
        return node_status;

    case kParamCH1Status:
        return output_status[0];

    case kParamCH2Status:
        return output_status[1];

    case kParamCH3Status:
        return output_status[2];

    case kParamCH4Status:
        return output_status[3];

    case kParamCH5Status:
        return output_status[4];

    case kParamCH1Current:
        return current[0];

    case kParamCH2Current:
        return current[1];

    case kParamCH3Current:
        return current[2];

    case kParamCH4Current:
        return current[3];

    case kParamCH5Current:
        return current[4];

    case kParamInfo0 ... kParamInfo7:
        return info[address() - kParamInfo0];

    case Parameter::configBase ... Parameter::configTop:
        return eeprom_read_word((const uint16_t *)((address() - Parameter::configBase) * 2));

    case Parameter::defaultBase ... Parameter::defaultTop:
        return param_default(address() - Parameter::defaultBase + Parameter::configBase);

    }

    return 0;
}

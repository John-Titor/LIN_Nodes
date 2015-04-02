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
#include <avr/io.h>
#include <avr/eeprom.h>

#include "board.h"
#include "timer.h"

#include "master.h"
#include "switches.h"
#include "relays.h"

#include "lin_defs.h"

bool __cxa_guard_acquire() { return true; }
void __cxa_guard_release() {}

using namespace Master;

MasterNode  gMaster;

static uint16_t free_memory;

static void
param_init()
{
    uint8_t options = eeprom_read_byte((uint8_t *)Board::kConfigOptions);
    bool should_default = (options & Board::kOptResetConfig);

    // init parameters (set to defaults if requested or not valid
    for (Parameter::Address addr = Generic::kParamConfigBase; addr < Generic::kParamConfigTop; addr++) {
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

void
main(void)
{
    Board::early_init();

    // check for recovery mode before doing anything else
    if (Board::get_mode() != 0) {
        Board::panic(Board::kPanicCodeRecovery);
    }

    // initialisation
    param_init();
    gMaster.init();         // on v1 boards, must do this before SPI due to !SS being LINCS
    Switches::init();

    // enable interrupts; timers and LIN events will start.
    sei();

    // run the master logic forever
    for (;;) {
        wdt_reset();

        // update free memory value each time it's read
        if (free_memory == 0) {
            free_memory = Board::freemem();
        }

        // scan the switch array
        Switches::scan();

        // Run the relay logic, update our idea of what the vehicle state should be.
        // Results are broadcast routinely anytime the vehicle is awake.
        Relays::tick();

    }
}

void
Parameter::set(uint16_t value) const
{
    switch (address()) {
    case Generic::kParamOperationMode:
        switch (value) {
        case operation_magic::kEnterBootloader:
            Board::enter_bootloader(Master::kNodeAddress, board_function::kMaster);
            
        case operation_magic::kSetDefaults:
            Board::reset_to_defaults();
        }

    case Generic::kParamConfigBase ... Generic::kParamConfigTop:
        eeprom_update_word((uint16_t *)((address() - Generic::kParamConfigBase) * 2), value);
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
        return board_function::kMaster;

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

    case Generic::kParamConfigBase ... Generic::kParamConfigTop:
        return eeprom_read_word((const uint16_t *)((address() - Generic::kParamConfigBase) * 2));

    case Generic::kParamDefaultBase ... Generic::kParamDefaultTop:
        return param_default(address() - Generic::kParamDefaultBase + Generic::kParamConfigBase);

    }



    return 0;
}

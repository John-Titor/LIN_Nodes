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

// Board-specific functionality
//

#include <avr/power.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay_basic.h>

#include "board.h"

extern "C" uint8_t _end;

namespace Board
{

#ifdef DEBUG
/// Debug serial port
///
Serial       debugPort;
#endif

uint8_t wdt_reset_count;

void
early_init()
{
    // XXX save reset cause for later use?
    if (MCUSR & WDRF) {
        wdt_reset_count++;
    }

    MCUSR = 0;

    // reset watchdog timer to a more generous value
    wdt_enable(WDTO_500MS);
    wdt_reset();

    // set the prescaler for maximum speed
    clock_prescale_set(clock_div_1);

    // turn everything on
    power_all_enable();

    // fill (most of) the stack with 0xff for sniffing purposes
    volatile uint8_t *p = &_end;

    while (p < (uint8_t *)SP) {
        *p++ = 0xff;
    }
}


void
panic(uint8_t code)
{
#if defined (BOARD_BOOTLOADER)
    for (;;) {
        // wait for the watchdog
    }
#else
    debug("panic %3u @ %p", code, __builtin_return_address(0));

    // disable interrupts and wait for possible pending LIN transmit to
    // complete
    cli();
    ms_delay(10);

    // start in a state that doesn't risk powering us off
    pinLINTX.set();
    pinLINTX.cfg_output();
    pinLINCS.clear();
    pinLINCS.cfg_output();

    for (;;) {

        ms_delay(1000);

        // blink the LIN CS LED with our error code
        for (uint8_t i = 0; i < code; i++) {
            wdt_reset();
            pinLINCS.set();
            ms_delay(200);
            wdt_reset();
            pinLINCS.clear();
            ms_delay(200);
        }
    }
#endif
}

uint8_t
get_mode()
{
    uint8_t mode = 0;

    // configure Mode pins for reading
#ifdef pinMode1
    pinMode1.cfg_input_pullup();
#ifdef pinMode2
    pinMode2.cfg_input_pullup();
#ifdef pinMode4
    pinMode4.cfg_input_pullup();
#ifdef pinMode8
    pinMode8.cfg_input_pullup();
#endif
#endif
#endif
#endif


    // sample the mode pins
    ms_delay(10);                // allow inputs to settle

#ifdef pinMode1

    if (!pinMode1.get())        // 1 bits are pulled low
        mode |= 1;

#ifdef pinMode2

    if (!pinMode2.get())
        mode |= 2;

#ifdef pinMode4

    if (!pinMode4.get())
        mode |= 4;

#ifdef pinMode8

    if (!pinMode8.get())
        mode |= 8;

#endif
#endif
#endif
#endif

    return mode;
}

void
sleep()
{
    LINCR = 0;                  // stop the LIN block
    ms_delay(20);               // wait for any frame to complete (seems necessary to avoid hangs?)
    power_all_disable();        // turn off all of the peripherals we can

    // Put the board to sleep.
    //
    pinLINTX.set();         // make sure TX is 1
    pinLINCS.clear();       // make sure CS is 0
    pinLINCS.cfg_output();  // make sure CS is an output (will be low now)
    pinLINTX.cfg_output();  // make sure TX is an output (will be high now)
    pinLINTX.clear();       // drive TX low to turn off power
    for (;;) {
        // If we fail to power down, e.g. because LIN traffic causes the regulator to stay on,
        // the watchdog will pull us back out via the reset path after ~500ms.
    }
}

void
ms_delay(uint16_t ms)
{
    while (ms > 0) {
        wdt_reset();
        us_delay(1000);   // XXX approximate
        ms--;
    }

    wdt_reset();
}

void
us_delay(uint16_t us)
{

    // _delay_loop_2 consumes 4 cycles per count, so convert microseconds to
    // units of 4 cycles
    us *= F_CPU / 4000000UL;

    _delay_loop_2(us);
}

uint16_t
freemem()
{
    // approximate free memory value
    volatile uint8_t *p = &_end;
    uint16_t mem = 0;

    while ((*p == 0xff) && (p < (uint8_t *)SP)) {
        mem++;
        p++;
    }

    return mem;
}

void
enter_bootloader(uint8_t nad, uint8_t function)
{
    // make sure the watchdog doesn't catch us while we're updating the EEPROM
    wdt_reset();

    // write magic to the EEPROM to cause us to wait in the bootloader
    eeprom_update_byte((uint8_t *)kConfigNodeAddress, nad);
    eeprom_update_byte((uint8_t *)kConfigFunction, function);
    eeprom_update_word((uint16_t *)kConfigMagic, kBLMagic); // operation_magic::kEnterBootloader

    reset();
}

void
reset_to_defaults()
{
    // make sure the watchdog doesn't catch us while we're updating the EEPROM
    wdt_reset();

    // write magic to the EEPROM requesting parameter reset on reboot
    eeprom_update_byte((uint8_t *)Board::kConfigOptions, Board::kOptResetConfig);

    reset();
}

bool
was_bootloader_requested()
{
    bool requested = (eeprom_read_word((uint16_t *)Board::kConfigMagic) == Board::kBLMagic);
    eeprom_update_word((uint16_t *)Board::kConfigMagic, 0xffff);

    return requested;
}

void
reset()
{
    // shorten the watchdog timer down to the minimum so that we reset ASAP
    wdt_enable(WDTO_15MS);

    for (;;)
        ;
}

} // namespace Board

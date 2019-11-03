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

///@file board.h

///
/// Board pin definitions
///

#pragma once

#include <avr/io.h>

#include "pin.h"
#include "softserial.h"

#if defined (BOARD_BOOTLOADER)
# include "config_bootloader.h"
#elif defined(BOARD_POWER_V3)
# include "config_power_v3.h"
#elif defined(BOARD_MASTER_V1)
# include "config_master_v1.h"
#elif defined(BOARD_USB_LINK)
# include "config_usb_link.h"
#else
# error Need to define a board type
#endif

namespace Board
{
extern uint8_t wdt_reset_count;

/// Generic 'board' initialisation; clock/power setup, watchdog init, etc.
///
void early_init();

enum PanicCode : uint8_t {
    kPanicCodeRecovery  = 2,
    kPanicCodeSPI       = 3,
    kPanicCodeLIN       = 4,
    kPanicCodeAssert    = 5,
    kPanicCodeLowMemory = 6
};

enum EEPROMInfo : uint16_t {
    kInfoPage           = FLASHEND - (SPM_PAGESIZE - 1),

    kInfoProgramCRC     = kInfoPage + 0,    // CRC of program, used by bootloader
    kInfoResetVector    = kInfoPage + 2,    // program entrypoint, used by bootloader
    kInfoProgramEnd     = kInfoPage + 4,    // end of program, used by bootloader

    kConfigOptions      = (E2END - 4),      // see EEPROMOptions below
    kConfigNodeAddress  = (E2END - 3),      // LIN NAD for the node, must be legal for the loaded firmware, used by
                                            // bootloader and firmwares supporting more than one address
    kConfigFunction     = (E2END - 2),      // board function, must match the loaded firmware, used by bootloader
    kConfigMagic        = (E2END - 1),      // if == kBLMagic, enter bootloader on reset

    kBLMagic            = 0x4f42            // AKA operation_magic::kEnterBootloader
};

enum EEPROMOptions : uint8_t {
    kOptResetConfig     = (1 << 0)          // if set at startup, node firmware to reset configuration to defaults
};

/// Panic with a status code
///
void panic(uint8_t code) __attribute__((noreturn));

/// Read the board-specific mode configuration switch (if any)
///
uint8_t get_mode();

/// Put the board to sleep
///
void sleep();

/// Delay for a period in milliseconds, keeping the watchdog at bay
///
void ms_delay(uint16_t ms);

/// Delay for a period in microseconds
///
void us_delay(uint16_t us);

/// Check free space between stack and BSS
///
/// @return         The number of free bytes left between the BSS and stack
///
uint16_t freemem();

/// Reboot into the bootloader
///
void enter_bootloader(uint8_t nad, uint8_t function = 0) __attribute__((noreturn));

/// Reboot and reset parameters to default
void reset_to_defaults() __attribute__((noreturn));

/// Test whether the bootloader was requested
///
bool was_bootloader_requested();

/// Reset
///
void reset() __attribute__((noreturn));

/// Set the LIN CS state
///
static inline void
lin_CS(bool state)
{
    if (state) {
        pinLINCS.set();

    } else {
        pinLINCS.clear();
    }
}

#if defined(DEBUG) && defined(pinDebugTX)
extern Serial debugPort;
# define debug(fmt, args...)    Board::debugPort.printf(PSTR(fmt "\n"), ## args)
# define debugc(c)              Board::debugPort.putc(c)
#else
# define debug(fmt, args...)    do {} while(0)
# define debugc(c)              do {} while(0)
#endif

} //namespace Board

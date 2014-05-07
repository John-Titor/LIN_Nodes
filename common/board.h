///@file board.h
///
/// Board pin definitions
///

#pragma once

#include "pin.h"
#include "parameter.h"
#include "softserial.h"

#if defined(BOARD_POWER_V1)
# include "config_power_v1.h"
#endif
#if defined(BOARD_MASTER_V1)
# include "config_master_v1.h"
#endif
#if defined(BOARD_PROGRAMMER_V1)
# include "config_programmer_v1.h"
#endif
#ifndef pinLINRX
# error Need to define a board type
#endif

namespace Board
{
void init();

enum PanicCode : uint8_t {
    kPanicRecovery  = 2,
    kPanicI2C       = 3,
    kPanicSPI       = 4,
    kPanicLIN       = 5
};

/// Panic with a status code
///
void panic(uint8_t code) __attribute__((noreturn));

/// Read the board-specific mode configuration switch (if any)
///
uint8_t getMode();

/// Put the board to sleep
///
void sleep();

/// Delay for a period in milliseconds, keeping the watchdog at bay
///
void msDelay(uint16_t ms);

/// Delay for a period in microseconds
///
void usDelay(uint16_t us);

/// Check free space between stack and BSS
///
/// @return         The number of free bytes left between the BSS and stack
///
uint16_t freemem();

/// Set the LIN CS state
///
static inline void
linCS(bool state)
{
    if (state) {
        pinLINCS.set();
    } else {
        pinLINCS.clear();
    }
}

#if defined(DEBUG) && defined(pinDebugTX)
extern Serial debugPort;
# define debug(fmt, args...)    Board::debugPort.printfP(PSTR(fmt "\n"), ## args)
#else
# define debug(fmt, args)       do {} while(0)
#endif

} //namespace Board

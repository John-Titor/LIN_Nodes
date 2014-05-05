
#include <avr/io.h>

#include "mc33972.h"
#include "board.h"

namespace MC33972
{

enum Command : uint8_t {
    kCMDStatus          = 0x00,
    kCMDSettings        = 0x01,
    kCMDWakeup1         = 0x02,
    kCMDWakeup2         = 0x03,
    kCMDMetallic1       = 0x04,
    kCMDMetallic2       = 0x05,
    kCMDAnalog          = 0x06,
    kCMDWetting1        = 0x07,
    kCMDWetting2        = 0x08,
    kCMDTristate1       = 0x09,
    kCMDTristate2       = 0x0a,
    kCMDCalibration     = 0x0b,
    kCMDSleep           = 0x0c,
    kCMDReset           = 0x7f
};

uint8_t                 _buf[3];

static void transfer(uint8_t *buf);
static void cmd(Command cmd, uint8_t op1 = 0, uint8_t op2 = 0);
static void xferWait() { while (!(SPSR & (1 << SPIF))) {} }

void
init()
{
    // reset the chip to defaults
    cmd(kCMDReset);

    // all SP inputs switch-to-battery
    cmd(kCMDSettings, 0, 0xff);

    // all input pulls enabled
    cmd(kCMDTristate1, 0, 0);
    cmd(kCMDTristate2, 0, 0);

    // all wakeups disabled
    cmd(kCMDWakeup1, 0, 0);
    cmd(kCMDWakeup2, 0, 0);
}

void scan()
{
    cmd(kCMDStatus);
}

bool test(uint8_t inp)
{
    return (inp < kInputMax) && (_buf[inp / 8] & (1 << (inp & 0x7)));
}

void
transfer(uint8_t *buf)
{
    // clear flags ready for use
    (void)SPSR;
    (void)SPDR;

    // Configure SPI for mode 2 master at 2MHz
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << CPHA);

    // select the slave
    pinCS.clear();

    SPDR = buf[0];
    xferWait();
    _buf[2] = SPDR;

    SPDR = buf[1];
    xferWait();
    _buf[1] = SPDR;

    SPDR = buf[2];
    xferWait();
    _buf[0] = SPDR;

    pinCS.set();
}

void
cmd(Command cmd, uint8_t op1, uint8_t op2)
{
    uint8_t buf[] = { cmd, op1, op2};

    debug("cmd %2x", cmd);
    transfer(&buf[0]);
}

} // namespce MC33972
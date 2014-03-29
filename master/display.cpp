
#include <util/delay.h>

#include "USI_TWI_Master.h"
#include "display.h"

#include "board.h"

bool
Display::init()
{
    USI_TWI_Master_Initialise();

    // ping the display
    uint8_t pkt[] = { 0x00, 1, 0x5a };
    if (send(&pkt[0]) == FALSE) {
        return false;
    }
    _delay_ms(5);

    // check the ping response
    uint8_t buf[3];
    if ((recv(&buf[0], 3) == FALSE) || (buf[0] != 0x40) || (buf[1] != 1) || (buf[2] != 0x5a)) {
        return false;
    }
    clear();
    return true;
}

void
Display::clear()
{
    uint8_t pkt[] = { kOPClear, 0 };
    send(&pkt[0]);
    waitAck(kOPClear);
}

void
Display::setBacklight(uint8_t value)
{
    uint8_t pkt[] = { kOPBacklight, 1, value };
    send(&pkt[0]);
    waitAck(kOPBacklight);
}

uint8_t
Display::getButtons()
{
    uint8_t pkt[] = { 0x18, 0 };
    send(&pkt[0]);

    uint8_t buf[7];
    recv(&buf[0], sizeof(buf));

    return buf[2];
}

void
Display::write(uint8_t x, uint8_t y, char c)
{
    _x = x;
    _y = y;
    write(c);
}

void
Display::write(char c)
{
    uint8_t pkt[] = { kOPWrite, 3, _x, _y, c };
    send(&pkt[0]);
    waitAck(kOPWrite);

    if (++_x >= kWidth) {
        _x = 0;
        if (++_y >= kHeight) {
            _y = 0;
        }
    }
}

void
Display::write(uint8_t x, uint8_t y, const char *s)
{
    _x = x;
    _y = y;
    write(s);
}

void
Display::write(const char *s)
{
    while (*s != '\0') {
        write(*s++);
    }
}

void
Display::writeP(uint8_t x, uint8_t y, PGM_P s)
{
    _x = x;
    _y = y;
    writeP(s);
}

void
Display::writeP(PGM_P s)
{
    while (*s != '\0') {
        write(pgm_read_byte(s++));
    }
}

bool
Display::send(const uint8_t *pkt)
{
    uint8_t len = pkt[1] + 5;
    uint8_t buf[len];

    buf[0] = kWriteAddress;

    // copy packet
    for (uint8_t i = 0; i < (len - 3); i++) {
        buf[1 + i] = pkt[i];
    }

    // generate packet CRC
    crc(buf + 1);

    return USI_TWI_Start_Transceiver_With_Data(buf, len) == TRUE;
}

bool
Display::recv(uint8_t *pkt, uint8_t pktlen)
{
    bool result;
    uint8_t len = pktlen + 3;
    uint8_t buf[len];

    buf[0] = kReadAddress;

    result = USI_TWI_Start_Transceiver_With_Data(buf, len);

    if (result == TRUE) {
        for (uint8_t i = 0; i < pktlen; i++) {
            pkt[i] = buf[i + 1];
        }
    }
    return result;
}

bool
Display::waitAck(uint8_t opcode)
{
    uint8_t buf[2];

    if (!recv(&buf[0], 2)) {
        return false;
    }
    if (buf[0] != (opcode | kACK)) {
        return false;
    }
    return true;
}

void
Display::crc(uint8_t *ptr)
{
    uint8_t  count; // packet size
    uint16_t crc;   // calculated CRC
    uint8_t  i;     // loop count, bits in byte
    uint8_t  data;  // current byte being shifted

    crc = 0xFFFF;   // Preset to all 1's, prevent loss of leading zeros
    count = ptr[1] + 2; // get count from packet

    while (count--) {
        data = *ptr++;
        i = 8;
        do {
            if ((crc ^ data) & 0x01) {
                crc >>= 1;
                crc ^= 0x8408;
            } else {
                crc >>= 1;
            }
            data >>= 1;
        } while (--i != 0);
    }
    crc = ~crc;
    *ptr++ = crc & 0xff;
    *ptr++ = crc >> 8;
}

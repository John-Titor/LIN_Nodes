#include <stdarg.h>

#include <avr/pgmspace.h>

#include "print.h"

void
Print::write(const char *s)
{
    uint8_t c;

    while ((c = *s++) != 0) {
        _write(c);
    }
}

void
Print::writeP(PGM_P s)
{
    uint8_t c;

    while ((c = pgm_read_byte(s++)) != 0) {
        _write(c);
    }
}

void
Print::write(uint8_t n)
{
    _write(n, 3);
}

void
Print::write(uint16_t n)
{
    _write(n, 5);
}

void
Print::printfP(PGM_P fmt, ...)
{
    va_list ap;
    uint8_t c;

    va_start(ap, fmt);

    while ((c = pgm_read_byte(fmt++)) != 0) {

        // nl -> cr,nl
        if (c == '\n') {
            _write('\r');
        }

        // non-format characters
        if (c != '%') {
            _write(c);
            continue;
        }

        // initalise formatter
        uint8_t w = 0;

nextfmt:
        c = pgm_read_byte(fmt++);

        switch(c) {
        case '\0':                  // sanity
            return;

        case '1'...'9':
            w = c - '0';
            goto nextfmt;

        case 'p':
            _write('0');
            _write('x');
            /* FALLTHROUGH */
        case 'u':
            _write(va_arg(ap, unsigned), w ? w : 5);
            break;

        case 'x':
            _writex(va_arg(ap, unsigned), w ? w : 4);
            break;

        default:
            break;
        }
    }
}

void
Print::_write(uint16_t n, uint8_t width)
{
    char buf[width + 1];
    uint8_t pos = width;
    bool clear = false;

    buf[pos--] = 0;
    buf[pos] = '0';
    
    for (;;) {
        if (n > 0) {
            buf[pos] = '0' + n % 10;
            n /= 10;
        } else {
            if (clear) {
                buf[pos] = ' ';
            }
        }
        if (pos == 0) {
            break;
        }
        clear = true;
        pos--;
    } 
    write(&buf[0]);
}

void
Print::_writex(uint16_t n, uint8_t width)
{
    while (width > 0) {
        uint8_t d = (n >> 4 * (width - 1)) & 0xf;

        if (d <= 9) {
            _write('0' + d);
        } else {
            _write('a' + d - 10);
        }
        width--;
    }
}

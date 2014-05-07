#include <stdarg.h>

#include <avr/pgmspace.h>

#include "print.h"

//void
//Print::write(uint8_t n)
//{
//    _write(n, 3);
//}
//
//void
//Print::write(uint16_t n)
//{
//    _write(n, 5);
//}

void
Print::printf(PGM_P fmt, ...)
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
    char buf[width];
    uint8_t pos = width - 1;
    bool clear = false;

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
    while (pos < width) {
        putc(buf[pos++]);
    }
}

void
Print::_writex(uint16_t n, uint8_t width)
{
    uint8_t shift = 4 * (width - 1);

    for (;;) {
        uint8_t d = (n >> shift) & 0xf;

        if (d <= 9) {
            putc('0' + d);
        } else {
            putc('a' + d - 10);
        }
        if (shift == 0) {
            break;
        }
        shift -= 4;;
    }
}

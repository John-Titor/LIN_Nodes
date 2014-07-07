///@file m_edit.h

#pragma once

#include "util.h"

#include "menu.h"

namespace Menu
{

class EditMode : public Mode
{
public:
    EditMode() :
        _from(nullptr),
        _value(nullptr),
        _x(0),
        _y(0),
        _min(0),
        _max(0),
        _fmt(nullptr),
        _stringtab(nullptr)
    {}

    void    init(Mode *parent,
                 uint8_t *value,
                 uint8_t x,
                 uint8_t y,
                 uint8_t min = 0,
                 uint8_t max = 255,
                 const char *fmt = nullptr)
    {
        _from = parent;
        _value = value;
        _x = x;
        _y = y;
        _min = min;
        _max = max;
        _fmt = fmt ? fmt : PSTR("%3u");
        _stringtab = nullptr;

        if ((*_value) < _min) {
            (*_value) = _min;

        } else if ((*_value) > _max) {
            (*_value) = _max;
        }
    }

    void    init(Mode *parent,
                 uint8_t *value,
                 uint8_t x,
                 uint8_t y,
                 const char *stringtab,
                 const char *fmt = nullptr)
    {
        _from = parent;
        _value = value;
        _x = x;
        _y = y;
        _min = 0;
        _max = 255;
        _fmt = fmt ? fmt : "%3u";
        _stringtab = stringtab;

        if ((*_value) >= Util::strtablen(_stringtab)) {
            (*_value) = Util::strtablen(_stringtab);
        }
    }

    virtual Mode    *action(Encoder::Event bp) override;
    void            draw();

private:
    Mode            *_from;
    uint8_t         *_value;
    uint8_t         _x;
    uint8_t         _y;
    uint8_t         _min;
    uint8_t         _max;
    const char      *_fmt;
    const char      *_stringtab;

    void            increment();
    void            decrement();
};

extern EditMode  modeEdit;

} // namespace Menu

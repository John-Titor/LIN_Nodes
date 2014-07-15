#include <avr/wdt.h>

#include "lin_protocol.h"
#include "board.h"
#include "util.h"

#include "m_explore.h"
#include "m_top.h"
#include "m_setup.h"
#include "slave.h"

namespace Menu
{

ExploreMode modeExplore;

Bitarray<LIN::kNodeAddressMaxAssigned> ExploreMode::presentMask;

////////////////////////////////////////////////////////////////////////////////
// Explore mode
//
// +--------------------+
// |                    |
// | Searching... XX    |
// |                    |
// |                    |
// +--------------------+
//
// +--------------------+
// |Error:              |
// | Master node not    |
// | responding.        |
// |                    |
// +--------------------+
//
// +--------------------+
// |Select:             |
// |    -cancel-        |
// | >>Master           |
// |   Power #4         |
// +--------------------+
//
Mode *
ExploreMode::action(Encoder::Event bp)
{
    bool wantDraw = false;

    switch (bp) {

    case Encoder::kEventUp:
        _node = search_down(_node);
        Encoder::discard();
        wantDraw = true;
        break;

    case Encoder::kEventDown:
        _node = search_up(_node);
        Encoder::discard();
        wantDraw = true;
        break;

    case Encoder::kEventPress:
        switch (_node) {
        case 0:
            // force rescan on re-entry from top menu
            presentMask.reset();
            return &modeTop;

        case LIN::kNodeAddressMaster:
            modeSetupMaster.init();
            return &modeSetupMaster;

        case LIN::kNodeAddressPowerBase ...(LIN::kNodeAddressPowerBase + 15):
            modeSetupPower.init(_node);
            return &modeSetupPower;

        default:
            break;
        }

        break;

    case Encoder::kEventActivate:
        gDisplay.clear();

        // do we have a valid scan result with a master node? 
        // (i.e. are we re-entering from an edit state?)
        if (!presentMask.test(LIN::kNodeAddressMaster)) {
            presentMask.reset();
            presentMask.set((LIN::NodeAddress)0);   // used for the 'cancel' entry

            for (uint8_t i = LIN::kNodeAddressMaster; i < LIN::kNodeAddressMaxAssigned; i++) {
                gDisplay.clear();
                gDisplay.move(1, 1);
                gDisplay.printf(PSTR("Searching... %2u"), i);

                uint8_t dummy;

                if (gSlave.get_parameter(i, 0, dummy)) {
                    presentMask.set(i);
                }
            }

            if (!presentMask.test(LIN::kNodeAddressMaster)) {
                error(PSTR(" Master node not\n responding."));
                Board::ms_delay(3000);
                _node = 0;

            } else {
                _node = LIN::kNodeAddressMaster;
            }
        }

        wantDraw = true;
        break;

    default:
        break;
    }

    if (wantDraw) {
        draw();
    }

    return this;
}

void
ExploreMode::draw()
{
    uint8_t base;       // base for the list to be drawn
    uint8_t cursor;     // offset row for the cursor

    // To give the illusion of scrolling in a list, we need to look around the current
    // node a bit.
    if (search_down(_node) == _node) {
        base = _node;   // nothing earlier in the list
        cursor = 1;

    } else if ((search_up(_node) == _node) &&
               (search_down(_node) != search_down(search_down(_node)))) {
        base = search_down(search_down(_node));
        cursor = 3;

    } else {
        base = search_down(_node);
        cursor = 2;
    }

    gDisplay.clear();
    gDisplay.printf(PSTR("Select:"));

    for (uint8_t row = 1; row < 4; row++) {
        gDisplay.move(3, row);

        switch (base) {
        case 0:
            gDisplay.printf(PSTR("Back to Top Menu"));
            break;

        case 1:
            gDisplay.printf(PSTR("Master"));
            break;

        case LIN::kNodeAddressPowerBase ... LIN::kNodeAddressPowerBase+ 16:
            gDisplay.printf(PSTR("Power %2u"), base - LIN::kNodeAddressPowerBase);
            break;

        default:
            gDisplay.printf(PSTR("Node @ %2u"), base);
            break;
        }

        uint8_t next = search_up(base);

        if (next == base) {
            break;
        }

        base = next;
    }

    gDisplay.move(1, cursor);
    gDisplay.printf(PSTR(">>"));
}

uint8_t
ExploreMode::search_up(uint8_t from)
{
    for (uint8_t newNode = from + 1; newNode < LIN::kNodeAddressMaxAssigned; newNode++) {
        if (presentMask.test(newNode)) {
            return newNode;
        }
    }

    return from;
}

uint8_t
ExploreMode::search_down(uint8_t from)
{
    if (from > 0) {
        uint8_t newNode = from - 1;

        do {
            if (presentMask.test(newNode)) {
                return newNode;
            }
        } while (newNode-- >= 0);
    }

    return from;
}

} // namespace Menu

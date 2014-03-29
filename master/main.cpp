
#include <util/delay.h>
#include <avr/wdt.h>

#include "board.h"
#include "lin_protocol.h"

#include "display.h"
#include "slave.h"
#include "master.h"

Master          master;
Event           masterRequest(1, LIN::kFIDMasterRequest);
Event           slaveResponse(1, LIN::kFIDSlaveResponse);

MasterSlave     slave;
Display         disp;

int
main(void)
{
    // basic board & system init
    Board::init();
    slave.init();
    if (!disp.init()) {
        Board::panic(3);
    }
    disp.setBacklight(10);
    disp.write("Master");

    // XXX for now, idle here
    for (;;) {
        //disp.clear();
        //disp.write("test");
        _delay_ms(100);
        wdt_reset();
        pinLINCS.toggle();
    }
}


void
MasterSlave::headerReceived(LIN::FID fid)
{
    switch (fid) {
    case LIN::kFIDControls:
        // XXX send a 
    default:
        // we don't respond to the ususal slave messages
        break;
    }
}


void
MasterSlave::responseReceived(LIN::FID fid, LIN::Frame &frame)
{
    switch (fid) {

    default:
        // we don't respond to the ususal slave messages
        break;
    }
}


#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "lin_dev.h"
#include "board.h"

static LINDev *_dev;

ISR(LIN_TC_vect)
{
    _dev->isr_TC();
}

ISR(LIN_ERR_vect)
{
    _dev->isr_error();
}

LINDev::LINDev()
{
    _dev = this;
}

void
LINDev::init()
{
    // LINCS/LINTX start tristated, configure for LIN use.
    // XXX should be in the ctor...
    pinLINCS.set();
    pinLINCS.cfg_output();
    pinLINTX.cfg_input_pullup();
    pinLINRX.cfg_input_pullup();

    reinit();
}

void
LINDev::reinit()
{
    // Reset the LIN block
    Lin_full_reset();
    Lin_set_baudrate(CONF_LINBRR);
    Lin_2x_enable();

    // Enable interrupts
    Lin_set_enable_it();
}

void
LINDev::master_test()
{
    // XXX testing
    debug("LIN testing");

    // no interrupts
    Lin_clear_enable_it();

    // disable frame timeouts
    LINCR |= (1 << LCONF1);

    // no watchdog
    wdt_disable();

    for (;;) {
        mt_send_header(LIN::kFrameIDTest);

        while (!Is_lin_header_ready()) {
            if (LINSIR & (1 << LERR)) {
                debug("frame TX error %2x", Lin_get_error_status());
                Board::panic(Board::kPanicCodeLIN);
            }
        }

        if (Lin_get_id() != LIN::kFrameIDTest) {
            debug("FID mismatch, got %2x", Lin_get_id());
            Board::panic(Board::kPanicCodeLIN);
        }

        LIN::Frame f(1,2,3,4,5,6,7,8);
        st_send_response(f, 8);

        while (LINSIR & (1 << LBUSY)) {
            if (LINSIR & (1 << LERR)) {
                debug("frame TX error %2x", Lin_get_error_status());
                Board::panic(Board::kPanicCodeLIN);
            }
        }

        Board::ms_delay(100);
    }
}

void
LINDev::isr_TC() 
{
    if (Is_lin_header_ready()) {

        // ack the interrupt
        Lin_clear_idok_it();

        // handle the header
        st_header_received();

    }

    if (Is_lin_rx_response_ready()) {
        LIN::Frame f;

        // reset the FIFO readout pointer
        Lin_clear_index();

        // copy FIFO data into the frame
        for (uint8_t i = 0; i < 8; i++) {
            f[i] = Lin_get_data();
        }

        // reset to default mode, might have been in 1x mode
        // to handle this frame
        Lin_2x_set_type();

        // ack the interrupt
        Lin_clear_rxok_it();

        // handle the frame
        st_response_received(f);
    }

    if (LINSIR & (1 << LTXOK)) {

        st_response_sent();

        // and clear the interrupt
        Lin_clear_txok_it();
    }
}

void
LINDev::isr_error()
{

    if (Lin_get_error_status() & (1 << LBERR)) {
        errors[kErrorLine]++;
    }
    if (Lin_get_error_status() & (1 << LCERR)) {
        errors[kErrorChecksum]++;
    }
    if (Lin_get_error_status() & (1 << LPERR)) {
        errors[kErrorParity]++;
    }
    if (Lin_get_error_status() & (1 << LFERR)) {
        errors[kErrorFraming]++;
    }
    if (Lin_get_error_status() & (1 << LSERR)) {
        errors[kErrorSynchronisation]++;
    }

    // reset to default mode
    Lin_2x_enable();

    // clear the interrupt
    Lin_clear_err_it();
}

void
LINDev::st_expect_response(uint8_t length)
{
    if (LINSIR & (1 << LBUSY)) {
        debug("LIN busy when waiting for response LINSIR=%2x LINERR=%2x", LINSIR, Lin_get_error_status());
        Board::panic(Board::kPanicCodeLIN);
        return;
    }

    // select 1x mode for FIDs that require it
    if (current_FrameID() >= LIN::kFrameIDMasterRequest) {
        Lin_1x_set_type();
    } else {
        Lin_2x_set_type();
        Lin_set_rx_len(length);
    }

    // and configure to receive the response
    Lin_rx_response();
}

void
LINDev::mt_send_header(LIN::FrameID fid)
{
    // turn on the LIN transmitter - must not do this while we
    // might possibly still be transmitting
    Board::lin_CS(true);

    // forcibly reset the block to clear pending operation or any other stuck error
    reinit();
    if (LINSIR & (1 << LBUSY)) {
        debug("TX header while busy LINSIR=%2x LINERR=%2x", LINSIR, Lin_get_error_status());
        Board::panic(Board::kPanicCodeLIN);
    }

    // select 1x or 2x mode
    if (fid >= LIN::kFrameIDMasterRequest) {
        Lin_1x_enable();
        Lin_1x_set_id(fid);
        Lin_1x_set_len(8);
    } else {
        Lin_2x_set_id(fid);
    }

    // disable frame timeouts
    // XXX revisit?
    LINCR |= (1 << LCONF1);

    // send header
    Lin_tx_header();
}

void
LINDev::st_send_response(const LIN::Frame &f, uint8_t length)
{
    if (LINSIR & (1 << LBUSY)) {
        debug("TX response while busy LINSIR=%2x LINERR=%2x", LINSIR, LINERR);
        Board::panic(Board::kPanicCodeLIN);
        return;
    }

    // turn on the LIN transmitter
    Board::lin_CS(true);

    // select 1x mode for FIDs that require it
    if (current_FrameID() >= LIN::kFrameIDMasterRequest) {
        Lin_1x_set_type();
    } else {
        Lin_2x_set_type();
        Lin_set_tx_len(length);
    }

    // copy data to FIFO
    Lin_clear_index();
    for (uint8_t i = 0; i < length; i++) {
        Lin_set_data(f[i]);
    }

    // send frame
    Lin_tx_response();
}

void
LINDev::st_header_received()
{
}

void
LINDev::st_response_received(LIN::Frame &frame)
{
}

void
LINDev::st_response_sent()
{
}


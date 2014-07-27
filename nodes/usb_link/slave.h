/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <msmith@purgatory.org> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

///@file slave.h

#pragma once

#include "requests.h"
#include "lin_slave.h"
#include "lin_protocol.h"

class SlaveHistory
{
public:

    SlaveHistory() : 
        _savedFID(0),
        _FIDValid(false),
        _nextIn(0),
        _nextOut(0) 
    {
    }

    /// Save a received FID into the nextIn entry regardless of whether
    /// we are ready to push the remainder of the frame just yet.
    ///
    /// @param fid              A just-received FID
    ///
    void            sawFID(uint8_t fid);

    /// Push a just-received or just-about-to-be-sent response (plus the
    /// previously-saved FID) into the ring if there is space for it.
    ///
    /// @param f                The response to push.
    ///
    void            sawResponse(LIN::Frame &f);

    /// Get a pointer to the oldest frame in the queue.
    /// This will remain valid until at least the next poll cycle.
    ///
    /// @return                 true if a frame was pulled
    ///
    uint8_t         *get();

private:
    struct Entry {
        uint8_t     bytes[9];
    };

    static const uint8_t    _size = 8;
    Entry                   _entries[_size + 1];

    uint8_t                 _savedFID;
    bool                    _FIDValid;
    uint8_t                 _nextIn;
    volatile uint8_t        _nextOut;

    uint8_t         next(uint8_t index) const { return (index < _size) ? index + 1 : 0; }
    bool            empty() const { return _nextIn == _nextOut; }
    bool            full() const { return next(_nextIn) == _nextOut; }
};

class ToolSlave : public Slave
{
public:
    ToolSlave();

    uint8_t         *get_history() { return _history.get(); }

    void            get_data_by_id(uint8_t nad, uint8_t page, uint8_t index);
    void            set_data_by_id(uint8_t nad, uint8_t page, uint8_t index, uint16_t value);

    uint16_t        *get_data() { return &_dataValue; }

    bool            is_data_ready() const { return _state == kStateIdle; }
    bool            is_data_error() const { return _state == kStateError; }

protected:
    virtual void    st_header_received() override;
    virtual void    st_response_received(LIN::Frame &frame) override;
    virtual void    st_sleep_requested(SleepType type) override;
    virtual bool    st_master_request(LIN::Frame &frame) override;

private:
    enum State : uint8_t {
        kStateIdle,
        kStateError,

        kStateSetData,
        kStateGetData,
        kStateWaitData
    };

    SlaveHistory        _history;
    State               _state;

    uint8_t             _nodeAddress;
    uint8_t             _dataPage;
    uint8_t             _dataIndex;
    uint16_t            _dataValue;
};

///@file slave.h

#pragma once

#include "lin_slave.h"
#include "lin_protocol.h"

class ProgrammerSlave : public Slave
{
public:
    ProgrammerSlave() : Slave(LIN::kNADProgrammer) {}

    void            reset()         { _state = kStateIdle; }

    void            setParameter(LIN::NodeAddress nad, uint8_t param, uint8_t value);
    void            requestParameter(LIN::NodeAddress nad, uint8_t param);

    uint8_t         getParameter()          const { return _paramValue; }
    bool            isIdle()                const { return _state == kStateIdle; }
    bool            isError()               const { return _state == kStateError; }
    bool            isParameterAvailable()  const { return _state == kStateGetComplete; }

protected:
    virtual void    headerReceived(LIN::FID fid) override;
    virtual void    responseReceived(LIN::FID fid, LIN::Frame &frame) override;
    virtual void    responseSent() override;

private:
    enum State : uint8_t {
        kStateIdle,

        kStateSetWaitRequest,
        kStateSetWaitSent,

        kStateGetWaitRequest,
        kStateGetWaitSlaveResponse,
        kStateGetComplete,

        kStateError
    };

    volatile State      _state;
    volatile uint8_t    _paramValue;

    uint8_t             _nodeAddress;
    uint8_t             _paramIndex;
};
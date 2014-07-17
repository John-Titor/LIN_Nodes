
#include "board.h"
#include "bitarray.h"
#include "util.h"

#include "switches.h"
#include "master.h"

#include "protocol.h"
#include "param_Master.h"

namespace Relays
{

typedef Bitarray<64> RelayBits;

class TurnBlinker : public Timer
{
public:
    TurnBlinker() :
        Timer(blink, this),
        _state(false),
        _active(false),
        _count(0)
    {}
    bool        state() const { return _state; }
    void        start()
    {
        _state = true;
        _active = true;
        _count = paramPassingBlinkCount;
        _remaining = _interval = paramTurnBlinkPeriod * 10U;
    }
    void        stop()
    {
        _active = false;

        if (_count == 0) {
            cancel();
        }
    }
    void        cancel()
    {
        _state = false;
        _remaining = _interval = 0;
    }

private:
    volatile bool       _state: 1;
    volatile bool       _active: 1;
    volatile uint8_t    _count;

    static void blink(void *arg)
    {
        auto b = reinterpret_cast<TurnBlinker *>(arg);
        b->_blink();
    }

    void        _blink()
    {
        if (_state) {
            _state = false;

            if (_count > 0) {
                _count--;

                if (!_active && (_count == 0)) {
                    cancel();
                }
            }

        } else {
            _state = true;
        }
    }
};

class BrakeBlinker : public Timer
{
public:
    BrakeBlinker() :
        Timer(blink, this),
        _state(false),
        _count(0)
    {}

    bool        state() const { return _state; }
    void        start()
    {
        _state = true;
        _remaining = _interval = paramBrakeBlinkPeriod * 10U;
        _count = paramBrakeBlinkCount;
    }

private:
    volatile bool       _state: 1;
    uint8_t             _count;

    static void blink(void *arg)
    {
        auto b = reinterpret_cast<BrakeBlinker *>(arg);
        b->_blink();
    }
    void        _blink()
    {
        if (_state && (_count > 0)) {
            _state = false;
            _count--;

        } else {
            _state = true;

            if (_count == 0) {
                _remaining = 0;
            }
        }
    }
};

class WiperDelay : public Timer
{
public:
    WiperDelay() :
        Timer(swap, this),
        _state(false)
    {}

    void        reset() { _state = true; _swap(); }
    bool        state() const { return _state; }

private:
    bool        _state: 1;

    static void swap(void *arg)
    {
        auto t = reinterpret_cast<WiperDelay *>(arg);
        t->_swap();
    }
    void        _swap()
    {
        if (_state) {
            // wiper control signal off for the specified interval
            // XXX as feature, this should be based on an analog input...
            _remaining = paramWiperInterval * 100U;
            _state = false;

        } else {
            // wiper control signal on for 1/2 second, motor responsible
            // for completing wipe & parking
            _remaining = 500U;
            _state = true;
        }
    }
};

class StayAwakeTimer : public Decrementer
{
public:
    StayAwakeTimer() : Decrementer(kStayAwakeTime) {}

    void                reset() { set_milliseconds(kStayAwakeTime); }

private:
    static const Timer::Timeval kStayAwakeTime = 50000;
};

static TurnBlinker      turnBlinker;
static BrakeBlinker     brakeBlinker;
static StayAwakeTimer   awakeDelay;
static WiperDelay       wiperDelay;
static Decrementer      interiorLightsDelay;
static Decrementer      pathwayLightingDelay;

static void
powerSignals(RelayBits &f)
{
    // ignition on?
    if (Switches::test(kSwitchIDIgnition)) {
        f.set(kRelayIDIgnition);

        // start switch on?
        if (Switches::test(kSwitchIDStart)) {
            f.set(kRelayIDStart);
        }
    }
}

static void
markerLights(RelayBits &f)
{
    // markers and city lights?
    if (Switches::test(kSwitchIDMarkerLights)) {

        awakeDelay.reset();                     // markers are on, stay awake

        f.set(kRelayIDMarkers);

        // Note: overridden by turnSignals below
        f.set(kRelayIDLeftTurnMarker);
        f.set(kRelayIDRightTurnMarker);
    }
}

static void
turnSignals(RelayBits &f)
{
    // Note: if turnBlinker is disabled (period is zero) then
    // we assume that there is an external flasher in use, and
    // the switches will come and go under its control.

    // external blinker mode?
    if ((paramTurnBlinkPeriod == 0) ||
        (paramTurnBlinkPeriod > 100)) {

        if (Switches::test(kSwitchIDLeftTurn)) {
            f.set(kRelayIDLeftTurn);
            f.clear(kRelayIDLeftTurnMarker);
        }

        if (Switches::test(kSwitchIDRightTurn)) {
            f.set(kRelayIDRightTurn);
            f.clear(kRelayIDRightTurnMarker);
        }

        return;
    }

    // smart hazard warning lights?
    if (Switches::test(kSwitchIDHazard)) {

        awakeDelay.reset();                     // hazards are on, stay awake

        // start the blinker
        if (Switches::changed(kSwitchIDHazard)) {
            turnBlinker.start();
        }

        // both signals track the blinker
        if (turnBlinker.state()) {
            f.set(kRelayIDLeftTurn);
            f.set(kRelayIDRightTurn);
        }

        return;

    } else {

        // hazard blinker just turned off?
        if (Switches::changed(kSwitchIDHazard)) {
            turnBlinker.cancel();
        }

        // XXX this isn't quite right; if a smart turn signal was engaged while
        //     hazards were on, we will miss it...
    }

    // parking markers?
    if (!Switches::test(kSwitchIDIgnition)) {

        // parking lights - no blink
        if (Switches::test(kSwitchIDLeftTurn)) {
            awakeDelay.reset();                     // parking markers are on, stay awake
            f.set(kRelayIDLeftTurn);
        }

        if (Switches::test(kSwitchIDRightTurn)) {
            awakeDelay.reset();                     // parking markers are on, stay awake
            f.set(kRelayIDRightTurn);
        }

        return;
    }

    // smart turn signals?
    if (Switches::test(kSwitchIDIgnition)) {
        static bool blinkLeft;

        // cancel/restart blinker
        if (Switches::changed_to_on(kSwitchIDLeftTurn)) {
            turnBlinker.start();
            blinkLeft = true;
        }

        if (Switches::changed_to_on(kSwitchIDRightTurn)) {
            turnBlinker.start();
            blinkLeft = false;
        }

        // stop blinker
        if (Switches::changed_to_off(kSwitchIDLeftTurn) ||
            Switches::changed_to_off(kSwitchIDRightTurn)) {
            turnBlinker.stop();
        }

        // signals on to suit
        if (turnBlinker.state()) {
            f.set(blinkLeft ? kRelayIDLeftTurn : kRelayIDRightTurn);
            f.clear(blinkLeft ? kRelayIDLeftTurnMarker : kRelayIDRightTurnMarker);
        }
    }
}

static void
headLights(RelayBits &f)
{
    static bool highBeamToggle;

    // lights up if forced
    if (Switches::test(kSwitchIDLightsUp)) {
        f.set(kRelayIDLightsUp);

        // otherwise lights down if nothing is on

    } else if (!Switches::test(kSwitchIDMarkerLights) &&
               !Switches::test(kSwitchIDHeadLights) &&
               !Switches::test(kSwitchIDHighBeam)) {
        f.set(kRelayIDLightsDown);
    }

    // force lights up for test/maintenance

    // no headlights without ignition
    if (!Switches::test(kSwitchIDIgnition)) {
        return;
    }

    // clear the highbeam toggle when ignition turns on
    if (Switches::changed(kSwitchIDIgnition)) {
        highBeamToggle = false;
    }

    // handle the high-beam toggle input
    if (Switches::changed_to_on(kSwitchIDHighBeamToggle)) {
        highBeamToggle = !highBeamToggle;
    }

    // test for any headlight on
    if (Switches::test(kSwitchIDHeadLights) ||
        Switches::test(kSwitchIDHighBeam)) {

        // headlights and markers on and popups up
        f.set(kRelayIDHeadLights);
        f.set(kRelayIDMarkers);
        f.set(kRelayIDLightsUp);
        f.clear(kRelayIDLightsDown);         // for safety

        // test for high beam; headlights off while starting
        if (!Switches::test(kSwitchIDStart)) {
            if (Switches::test(kSwitchIDHighBeam) ||
                highBeamToggle) {
                f.set(kRelayIDHighBeam);

            } else {
                f.set(kRelayIDLowBeam);
            }
        }
    }

    // foglights
    // XXX require other lights?
    if (Switches::test(kSwitchIDFogLight)) {
        f.set(kRelayIDFogLights);
    }
}

static void
tailLights(RelayBits &f)
{
    // brake lights
    if (Switches::test(kSwitchIDBrake)) {

        // start the blinker when the switch comes on
        if (Switches::changed(kSwitchIDBrake)) {
            brakeBlinker.start();
        }

        // brake light on if the blinker & pedal both agree
        if (brakeBlinker.state()) {
            f.set(kRelayIDBrake);
        }
    }

    // reverse lights
    if (Switches::test(kSwitchIDReverse)) {
        f.set(kRelayIDReverse);
    }
}

static void
interiorLights(RelayBits &f)
{
    // door just closed - start interior lighting timer
    if (Switches::changed_to_off(kSwitchIDDoor)) {
        interiorLightsDelay.set_seconds(paramInteriorLightTime);
    }

    // if ignition is on, cancel interior light timer
    if (Switches::test(kSwitchIDIgnition)) {
        interiorLightsDelay.clear();
    }

    // interior light on?
    if (Switches::test(kSwitchIDDoor) ||          // door open or
        Switches::test(kSwitchIDInteriorLight) || // light switch on or
        !interiorLightsDelay.expired()) {        // timer not expired

        awakeDelay.reset();                     // lights are on, stay awake

        f.set(kRelayIDInteriorLight);

        // XXX nice to have a time limit on the interior light when
        //     ignition is off to prevent battery drain
    }
}

static void
pathLights(RelayBits &f)
{
    static bool ignitionWasOn;

    // detect ignition transition to off
    if (Switches::changed_to_off(kSwitchIDIgnition)) {

        // door already open?
        if (Switches::test(kSwitchIDDoor)) {
            // path lighting
            pathwayLightingDelay.set_seconds(paramPathLightTime);

        } else {
            ignitionWasOn = true;
        }
    }

    // door opens after ignition off
    if (Switches::changed_to_on(kSwitchIDDoor) &&
        ignitionWasOn) {

        // path lighting
        pathwayLightingDelay.set_seconds(paramPathLightTime);

        // XXX no path lighting after a 'false alarm' door opening
        //     might want to keep this set until sleep?
        ignitionWasOn = false;
    }

    // ignition off and alarm unlock changed?
    if (!Switches::test(kSwitchIDIgnition) &&
        Switches::changed(kSwitchIDDoorUnlock)) {

        // welcome lighting
        pathwayLightingDelay.set_seconds(paramWelcomeLightTime);
    }

    // path lights on?
    if (!pathwayLightingDelay.expired()) {

        awakeDelay.reset();                     // path lights are on, stay awake

        f.set(kRelayIDMarkers);
        // XXX other pathway lights? town lights?
    }
}

static void
climateControl(RelayBits f)
{
    if (Switches::test(kSwitchIDIgnition) &&
        !Switches::test(kSwitchIDStart)) {
        if (Switches::test(kSwitchIDCabinFan1)) {
            f.set(kRelayIDCabinFan1);
        }

        if (Switches::test(kSwitchIDCabinFan2)) {
            f.set(kRelayIDCabinFan2);
        }

        if (Switches::test(kSwitchIDCabinFan3)) {
            f.set(kRelayIDCabinFan3);
        }

        if (Switches::test(kSwitchIDRearDefrost)) {
            f.set(kRelayIDRearDefrost);
        }
    }
}

static void
windowWipers(RelayBits f)
{
    if (Switches::test(kSwitchIDIgnition) &&
        !Switches::test(kSwitchIDStart)) {

        // avoid issues with overlap between wiper switch settings
        if (Switches::changed_to_on(kSwitchIDWiperInt)) {
            wiperDelay.reset();
        }

        if (Switches::test(kSwitchIDWiperHigh)) {
            f.set(kRelayIDWiperHigh);

        } else if (Switches::test(kSwitchIDWiperLow)) {
            f.set(kRelayIDWiperLow);

        } else if (Switches::test(kSwitchIDWiperInt)) {
            if (wiperDelay.state()) {
                f.set(kRelayIDWiperLow);
            }
        }
    }
}

void
tick()
{
    // Wake up and reset the awake timer if:
    // - ignition is on
    // - hazard warning signals are on
    // - left or right parking markers are on
    // - doors are open
    // - remote lock/unlock signal changed
    //
    if (Switches::test(kSwitchIDIgnition) ||
        Switches::test(kSwitchIDHazard) ||
        Switches::test(kSwitchIDLeftTurn) ||
        Switches::test(kSwitchIDRightTurn) ||
        Switches::test(kSwitchIDDoor) ||
        Switches::changed(kSwitchIDDoorUnlock)) {

        awakeDelay.reset();
        gMaster.set_sleep_enable(false);
    }

    // If the awake timer has expired, allow the master
    // to sleep and do nothing more here.
    if (awakeDelay.expired()) {
        gMaster.set_sleep_enable(true);
        return;
    }

    // update the relays frame by looking at switches
    RelayBits f;
    f.reset();

    // order here is important
    powerSignals(f);
    markerLights(f);
    turnSignals(f);
    headLights(f);
    tailLights(f);
    interiorLights(f);
    pathLights(f);
    climateControl(f);
    windowWipers(f);

    // update the copy we are sending to nodes
    gMaster.relayFrame.copy(f);

}

} // namespace Relays

/*
 * Copyright (c) 2012-2015, Mike Smith, <msmith@purgatory.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * o Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <avr/interrupt.h>

#include "board.h"
#include "bitarray.h"
#include "util.h"

#include "switches.h"
#include "master.h"

#include "lin_defs.h"

using namespace Master;

namespace Relays
{

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
        _count = Parameter(kParamPassingBlinkCount).get();
        _remaining = _interval = Parameter(kParamTurnBlinkPeriod).get();
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
            _remaining = Parameter(kParamWiperInterval).get();
            _state = false;

        } else {
            // wiper control signal on for 1/2 second, motor responsible
            // for completing wipe & parking.
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
    static const Timer::Timeval kStayAwakeTime = 20000U;
};

static TurnBlinker      turnBlinker;
static StayAwakeTimer   awakeDelay;
static WiperDelay       wiperDelay;
static Decrementer      interiorLightsDelay;
static Decrementer      pathwayLightingDelay;

static void
powerSignals(Response &resp)
{
    // ignition on?
    if (Switches::test(input_assignment::kIgnition)) {
        resp.Relays.Ignition = 1;

        // start switch on?
        if (Switches::test(input_assignment::kStart)) {
            resp.Relays.Start = 1;
        }
    }
}

static void
markerLights(Response &resp)
{
    // markers and city lights?
    if (Switches::test(input_assignment::kMarkerLights)) {

        awakeDelay.reset();                     // markers are on, stay awake

        resp.Relays.MarkerLights = 1;

        // Note: overridden by turnSignals below
        resp.Relays.LeftTurnMarker = 1;
        resp.Relays.RightTurnMarker = 1;
    }
}

static void
turnSignals(Response &resp)
{
    // Note: if turnBlinker is disabled (period is zero) then
    // we assume that there is an external flasher in use, and
    // the switches will come and go under its control.

    // external blinker mode?
    if (Parameter(kParamTurnBlinkPeriod).get() == 0) {

        if (Switches::test(input_assignment::kLeftTurn)) {
            resp.Relays.LeftTurn = 1;
            resp.Relays.LeftTurnMarker = 0;
        }

        if (Switches::test(input_assignment::kRightTurn)) {
            resp.Relays.RightTurn = 1;
            resp.Relays.RightTurnMarker = 0;
        }

        return;
    }

    // smart hazard warning lights?
    if (Switches::test(input_assignment::kHazard)) {

        awakeDelay.reset();                     // hazards are on, stay awake

        // start the blinker
        if (Switches::changed(input_assignment::kHazard)) {
            turnBlinker.start();
        }

        // both signals track the blinker
        if (turnBlinker.state()) {
            resp.Relays.LeftTurn = 1;
            resp.Relays.RightTurn = 1;
        }

        return;

    } else {

        // hazard blinker just turned off?
        if (Switches::changed(input_assignment::kHazard)) {
            turnBlinker.cancel();
        }

        // XXX this isn't quite right; if a smart turn signal was engaged while
        //     hazards were on, we will miss it...
    }

    // parking markers?
    if (!Switches::test(input_assignment::kIgnition)) {

        // parking lights - no blink
        if (Switches::test(input_assignment::kLeftTurn)) {
            awakeDelay.reset();                     // parking markers are on, stay awake
            resp.Relays.LeftTurn = 1;
        }

        if (Switches::test(input_assignment::kRightTurn)) {
            awakeDelay.reset();                     // parking markers are on, stay awake
            resp.Relays.RightTurn = 1;
        }

        return;
    }

    // smart turn signals?
    if (Switches::test(input_assignment::kIgnition)) {
        static bool blinkLeft;

        // cancel/restart blinker
        if (Switches::changed_to_on(input_assignment::kLeftTurn)) {
            turnBlinker.start();
            blinkLeft = true;
        }

        if (Switches::changed_to_on(input_assignment::kRightTurn)) {
            turnBlinker.start();
            blinkLeft = false;
        }

        // stop blinker
        if (Switches::changed_to_off(input_assignment::kLeftTurn) ||
            Switches::changed_to_off(input_assignment::kRightTurn)) {
            turnBlinker.stop();
        }

        // signals on to suit
        if (turnBlinker.state()) {
            if (blinkLeft) {
                resp.Relays.LeftTurn = 1;
                resp.Relays.RightTurn = 0;

            } else {
                resp.Relays.RightTurn = 1;
                resp.Relays.LeftTurn = 0;
            }
        }
    }
}

static void
headLights(Response &resp)
{
    static bool highBeamToggle;

    // lights up if forced
    if (Switches::test(input_assignment::kLightsUp)) {
        resp.Relays.LightsUp = 1;

        // otherwise lights down if nothing is on

    } else if (!Switches::test(input_assignment::kMarkerLights) &&
               !Switches::test(input_assignment::kHeadLights) &&
               !Switches::test(input_assignment::kHighBeam)) {

        // Note that it's correct to de-assert LightsDown (but not assert
        // LightsUp) with marker lights on; this allows a mode where the lights
        // are up but not lit for e.g. cleaning purposes.
        resp.Relays.LightsDown = 1;
    }

    // force lights up for test/maintenance

    // no headlights without ignition
    if (!Switches::test(input_assignment::kIgnition)) {
        return;
    }

    // clear the highbeam toggle when ignition turns on
    if (Switches::changed(input_assignment::kIgnition)) {
        highBeamToggle = false;
    }

    // handle the high-beam toggle input
    if (Switches::changed_to_on(input_assignment::kHighBeamToggle)) {
        highBeamToggle = !highBeamToggle;
    }

    // test for any headlight on
    if (Switches::test(input_assignment::kHeadLights) ||
        Switches::test(input_assignment::kHighBeam)) {

        // headlights and markers on and popups up
        resp.Relays.HeadLights = 1;
        resp.Relays.MarkerLights = 1;
        resp.Relays.LightsUp = 1;
        resp.Relays.LightsDown = 0;         // for safety

        // test for high beam; headlights off while starting
        if (!Switches::test(input_assignment::kStart)) {
            if (Switches::test(input_assignment::kHighBeam) ||
                highBeamToggle) {
                resp.Relays.HighBeam = 1;

            } else {
                resp.Relays.LowBeam = 1;
            }
        }
    }

    // foglights
    // XXX require other lights?
    if (Switches::test(input_assignment::kFogLight)) {
        resp.Relays.FogLights = 1;
    }
}

static void
tailLights(Response &resp)
{
    resp.Relays.Brake = 0;
    resp.Relays.Reverse = 0;

    if (Switches::test(input_assignment::kIgnition)) {

        // brake lights
        if (Switches::test(input_assignment::kBrake)) {

            resp.Relays.Brake = 1;
        }

        // reverse lights
        if (Switches::test(input_assignment::kReverse)) {
            resp.Relays.Reverse = 1;
        }
    }
}

static void
interiorLights(Response &resp)
{
    // door just closed - start interior lighting timer
    if (Switches::changed_to_off(input_assignment::kDoor)) {
        interiorLightsDelay.set_seconds(Parameter(kParamInteriorLightTime).get());
    }

    // if ignition is on, cancel interior light timer
    if (Switches::test(input_assignment::kIgnition)) {
        interiorLightsDelay.clear();
    }

    // interior light on?
    if (Switches::test(input_assignment::kDoor) ||          // door open or
        Switches::test(input_assignment::kInteriorLight) || // light switch on or
        !interiorLightsDelay.expired()) {        // timer not expired

        awakeDelay.reset();                     // lights are on, stay awake

        resp.Relays.InteriorLight = 1;

        // XXX nice to have a time limit on the interior light when
        //     ignition is off to prevent battery drain
    }
}

static void
pathLights(Response &resp)
{
    static bool ignitionWasOn;

    // detect ignition transition to off
    if (Switches::changed_to_off(input_assignment::kIgnition)) {

        // door already open?
        if (Switches::test(input_assignment::kDoor)) {
            // path lighting
            pathwayLightingDelay.set_seconds(Parameter(kParamPathLightTime).get());

        } else {
            ignitionWasOn = true;
        }
    }

    // door opens after ignition off
    if (Switches::changed_to_on(input_assignment::kDoor) &&
        ignitionWasOn) {

        // path lighting
        pathwayLightingDelay.set_seconds(Parameter(kParamPathLightTime).get());

        // XXX no path lighting after a 'false alarm' door opening
        //     might want to keep this set until sleep?
        ignitionWasOn = false;
    }

    // ignition off and alarm unlock changed?
    if (!Switches::test(input_assignment::kIgnition) &&
        Switches::changed(input_assignment::kDoorUnlock)) {

        // welcome lighting
        pathwayLightingDelay.set_seconds(Parameter(kParamWelcomeLightTime).get());
    }

    // path lights on?
    if (!pathwayLightingDelay.expired()) {

        awakeDelay.reset();                     // path lights are on, stay awake

        resp.Relays.MarkerLights = 1;
        // XXX other pathway lights? town lights?
    }
}

static void
climateControl(Response &resp)
{
    if (Switches::test(input_assignment::kIgnition) &&
        !Switches::test(input_assignment::kStart)) {
        if (Switches::test(input_assignment::kCabinFan1)) {
            resp.Relays.CabinFan1 = 1;
        }

        if (Switches::test(input_assignment::kCabinFan2)) {
            resp.Relays.CabinFan2 = 1;
        }

        if (Switches::test(input_assignment::kCabinFan3)) {
            resp.Relays.CabinFan3 = 1;
        }

        if (Switches::test(input_assignment::kRearDefrost)) {
            resp.Relays.RearDefrost = 1;
        }
    }
}

static void
windowWipers(Response &resp)
{
    if (Switches::test(input_assignment::kIgnition) &&
        !Switches::test(input_assignment::kStart)) {

        // avoid issues with overlap between wiper switch settings
        if (Switches::changed_to_on(input_assignment::kWiperInt)) {
            wiperDelay.reset();
        }

        if (Switches::test(input_assignment::kWiperHigh)) {
            resp.Relays.WiperHigh = 1;

        } else if (Switches::test(input_assignment::kWiperLow)) {
            resp.Relays.WiperLow = 1;

        } else if (Switches::test(input_assignment::kWiperInt)) {
            if (wiperDelay.state()) {
                resp.Relays.WiperLow = 1;
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
    if (Switches::test(input_assignment::kIgnition) ||
        Switches::test(input_assignment::kHazard) ||
        Switches::test(input_assignment::kLeftTurn) ||
        Switches::test(input_assignment::kRightTurn) ||
        Switches::test(input_assignment::kDoor) ||
        Switches::changed(input_assignment::kDoorUnlock)) {

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
    Response f;

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

    // atomically update the copy we are sending to nodes
    cli();
    gMaster.relayFrame = f;
    sei();

}

} // namespace Relays

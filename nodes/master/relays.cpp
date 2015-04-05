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

// Convenience macros

#define PARAMETER(_x)               Parameter(kParam##_x).get()
#define SWITCH(_x)                  Switches::test(input_assignment::k##_x)
#define SWITCH_CHANGED_TO_ON(_x)    Switches::changed_to_on(input_assignment::k##_x)
#define SWITCH_CHANGED_TO_OFF(_x)   Switches::changed_to_off(input_assignment::k##_x)
#define SWITCH_CHANGED(_x)          Switches::changed(input_assignment::k##_x)
#define RELAY(_x)                   resp.Relays._x

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
    bool        active() const { return _active; }

    void        start()
    {
        _state = true;
        _active = true;
        _count = PARAMETER(PassingBlinkCount);
        _remaining = _interval = PARAMETER(TurnBlinkPeriod);
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
            _remaining = PARAMETER(WiperInterval);
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

    void        reset() { set_milliseconds(kStayAwakeTime); }

private:
    static const Timer::Timeval kStayAwakeTime = 20000U;
};

static TurnBlinker      turnLeftBlinker;
static TurnBlinker      turnRightBlinker;
static StayAwakeTimer   awakeDelay;
static WiperDelay       wiperDelay;
static Decrementer      interiorLightsDelay;
static Decrementer      pathwayLightingDelay;


static void
headLights(Response &resp)
{
    static bool highBeamToggle;

    // clear the highbeam toggle when ignition turns on
    if (SWITCH_CHANGED(Ignition)) {
        highBeamToggle = false;
    }

    // toggle high beams if ignition is on and toggle is on
    if (SWITCH(Ignition) && SWITCH_CHANGED_TO_ON(HighBeamToggle)) {
        highBeamToggle = !highBeamToggle;
    }

    // Headlights
    // Feature #127
    //
    RELAY(HeadLights) = RELAY(Ignition) && (SWITCH(HeadLights) || SWITCH(HighBeam)) && !RELAY(Start);
    RELAY(LowBeam) = RELAY(Ignition) && !RELAY(Start) && SWITCH(HeadLights) && !SWITCH(HighBeam) && !highBeamToggle;
                     RELAY(HighBeam) = (RELAY(Ignition) && !RELAY(Start) && SWITCH(HighBeam)) ||
                                       (RELAY(Ignition) && !RELAY(Start) && SWITCH(HeadLights) && highBeamToggle);
    RELAY(FogLights) = RELAY(Ignition) && !RELAY(Start) && RELAY(HeadLights) && SWITCH(FogLight);
}

static void
turnSignals(Response &resp)
{
    // If TurnBlinkPeriod is zero then we assume that there
    // is an external flasher in use, and the inputs will come and
    // go under its control.
    //
    if (PARAMETER(TurnBlinkPeriod) == 0) {

        if (SWITCH(LeftTurn)) {
            RELAY(LeftTurn) = 1;
        }

        if (SWITCH(RightTurn)) {
            RELAY(RightTurn) = 1;
        }

        return;
    }

    // Turn signals
    // Feature #125
    //

    if (SWITCH_CHANGED_TO_OFF(Hazard)) {
        // hazard warning just turned off, cancel both signals
        turnLeftBlinker.cancel();
        turnRightBlinker.cancel();

    }

    if (!RELAY(Ignition) && !SWITCH(Hazard)) {
        // ignition is off and no hazard warning
        turnLeftBlinker.cancel();
        turnRightBlinker.cancel();

    }

    if (!SWITCH(Hazard) && !SWITCH(LeftTurn)) {
        // no hazard warning, left turn signal not on
        turnLeftBlinker.stop();
    }

    if (!SWITCH(Hazard) && !SWITCH(RightTurn)) {
        // no hazard warning, right turn signal not on
        turnRightBlinker.stop();
    }


    if (SWITCH_CHANGED_TO_ON(Hazard)) {
        // hazard warning just turned on, (re)start both signals
        turnLeftBlinker.start();
        turnRightBlinker.start();

    }

    if (RELAY(Ignition) && !SWITCH(Hazard) && SWITCH(LeftTurn) && !turnLeftBlinker.active()) {
        // left turn signal start
        turnRightBlinker.cancel();
        turnLeftBlinker.start();

    }

    if (RELAY(Ignition) && !SWITCH(Hazard) && SWITCH(RightTurn) && !turnRightBlinker.active()) {
        // right turn signal start
        turnLeftBlinker.cancel();
        turnRightBlinker.start();
    }


    RELAY(LeftTurn) = (!RELAY(Ignition) && !SWITCH(Hazard) && SWITCH(LeftTurn)) ||
                      (SWITCH(Hazard) && turnLeftBlinker.state()) ||
                      (RELAY(Ignition) && turnLeftBlinker.state());
    RELAY(LeftTurn) = (!RELAY(Ignition) && !SWITCH(Hazard) && SWITCH(LeftTurn)) ||
                      (SWITCH(Hazard) && turnLeftBlinker.state()) ||
                      (RELAY(Ignition) && turnLeftBlinker.state());
}

static void
markerLights(Response &resp)
{

    // Marker lights and pathway lighting
    // Feature #124, #106
    //

    static bool ignitionWasOn;

    // detect ignition transition to off
    if (SWITCH_CHANGED_TO_OFF(Ignition)) {

        // door already open?
        if (SWITCH(Door)) {
            pathwayLightingDelay.set_seconds(PARAMETER(PathLightTime));

        } else {
            ignitionWasOn = true;
        }
    }

    // door opens after ignition off
    if (SWITCH_CHANGED_TO_ON(Door) && ignitionWasOn) {

        // path lighting
        pathwayLightingDelay.set_seconds(PARAMETER(PathLightTime));

        // XXX no path lighting after a 'false alarm' door opening
        //     might want to keep this set until sleep?
        ignitionWasOn = false;
    }

    // ignition off and alarm unlock changed?
    if (!RELAY(Ignition) && SWITCH_CHANGED(DoorUnlock)) {

        // welcome lighting
        pathwayLightingDelay.set_seconds(PARAMETER(WelcomeLightTime));
    }

    // ignition on?
    if (RELAY(Ignition)) {
        pathwayLightingDelay.clear();
    }

    RELAY(MarkerLights) = (!RELAY(Ignition) && !RELAY(HeadLights) && !pathwayLightingDelay.expired()) ||
                          SWITCH(MarkerLights) ||
                          (RELAY(Ignition) && RELAY(HeadLights));
    RELAY(LeftTurnMarker) = RELAY(MarkerLights) ^ RELAY(LeftTurn);
    RELAY(RightTurnMarker) = RELAY(MarkerLights) ^ RELAY(RightTurn);

}

static void
windowWipers(Response &resp)
{
    // Window wipers
    // Feature #111
    //
    if (SWITCH_CHANGED_TO_ON(WiperInt)) {
        wiperDelay.reset();
    }

    RELAY(WiperHigh) = RELAY(Ignition) && !RELAY(Start) && SWITCH(WiperHigh);
    RELAY(WiperLow) = RELAY(Ignition) &&
                      (SWITCH(WiperLow) || wiperDelay.state()) &&
                      !RELAY(WiperHigh) &&
                      !RELAY(Start);
}

static void
interiorLights(Response &resp)
{
    // Interior lights
    // Feature #107
    //

    // door just closed - start interior lighting timer
    if (SWITCH_CHANGED_TO_OFF(Door)) {
        interiorLightsDelay.set_seconds(PARAMETER(InteriorLightTime));
    }

    // if ignition is on, cancel interior light timer
    if (RELAY(Ignition)) {
        interiorLightsDelay.clear();
    }

    RELAY(InteriorLight) = !interiorLightsDelay.expired() ||
                           SWITCH(InteriorLight) ||
                           SWITCH(Door);
}

void
tick()
{
    // Stay awake if we have a reason for it
    // Features #105, #108
    //
    if (SWITCH(Ignition)                ||  // ignition is on
        SWITCH(Hazard)                  ||  // hazard warning is on
        SWITCH(MarkerLights)            ||  // marker lights are on
        SWITCH(LeftTurn)                ||  // left parking markers are on
        SWITCH(RightTurn)               ||  // right parking markers are on
        SWITCH(Door)                    ||  // door is open
        SWITCH(InteriorLight)           ||  // interior light is on
        SWITCH_CHANGED(DoorUnlock)      ||  // doors locked/unlocked
        !pathwayLightingDelay.expired() ||  // pathway lighting still on
        // XXX TBD interior light timer (RELAY(InteriorLight)?)
        0) {

        awakeDelay.reset();
        gMaster.set_sleep_enable(false);

    } else {

        // If the awake timer has expired, allow the master
        // to sleep and do nothing more here.
        // Feature #108
        //
        if (awakeDelay.expired()) {
            gMaster.set_sleep_enable(true);
            return;
        }
    }

    // Update the relays frame by looking at switches
    //
    // Note that in the logic here, RELAY() values must not be used until they have
    // been computed.
    //
    Response resp;

    // Power signals
    // Feature #129
    //
    RELAY(Ignition) = SWITCH(Ignition);
    RELAY(Start) = RELAY(Ignition) && SWITCH(Start);

    // Tail lights
    // Feature #128
    //
    RELAY(Brake) = RELAY(Ignition) && SWITCH(Brake);
    RELAY(Reverse) = RELAY(Ignition) && SWITCH(Reverse);

    // Headlights
    //
    headLights(resp);

    // Turn signals
    //
    turnSignals(resp);

    // Marker lights
    //
    markerLights(resp);

    // Pop-up lights
    // Feature #116
    //
    RELAY(LightsDown) = (!RELAY(Ignition) && !RELAY(Start) && !SWITCH(LightsUp) && !RELAY(HeadLights)) ||
                        (RELAY(Ignition) && !RELAY(Start) && !SWITCH(LightsUp) && !RELAY(HeadLights));
    RELAY(LightsUp) = (!RELAY(Start) && SWITCH(LightsUp)) ||
                      (RELAY(Ignition) && !RELAY(Start) && RELAY(HeadLights));

    // Window wipers
    //
    windowWipers(resp);

    // Interior lights
    //
    interiorLights(resp);


    // atomically update the copy we are sending to nodes
    cli();
    gMaster.relayFrame = resp;
    sei();

}

} // namespace Relays

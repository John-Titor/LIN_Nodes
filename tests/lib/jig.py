#!/usr/bin/env python

"""
Copyright (c) 2012-2015, Mike Smith, <msmith@purgatory.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

o Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

o Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""

# Adjust these to match the actual serial numbers of the intended devices
MiscIOSerial = 324899
RelaySerial = 313561

PULL_TO_GROUND = 1
PULL_TO_BATTERY = 2
PULL_TO_NONE = 3

import time

from Phidget22.PhidgetException import *
from Phidget22.Phidget import *
from Phidget22.Unit import *
from Phidget22.VoltageSensorType import *
from Phidget22.VoltageRatioSensorType import *

from Phidget22.Devices.DigitalOutput import *
from Phidget22.Devices.VoltageInput import *
from Phidget22.Devices.VoltageRatioInput import *

class JigException(Exception):
    pass

class jigOutput(object):
    '''generic digital output on the jig'''

    def __init__(self, provider, channel, defaultState):
        self._defaultState = defaultState
        self.do = DigitalOutput()
        self.do.setDeviceSerialNumber(provider)
        self.do.setChannel(channel)
        self.do.setOnAttachHandler(self._attached)
        self.do.openWaitForAttachment(1000)

    def _attached(self, event):
        self.set(self._defaultState)

    @property
    def state(self):
        return self.do.getState()

    def set(self, state):
        self.do.setState(state)

    def reset(self):
        self.set(self._defaultState)


class jigSGOutput(jigOutput):

    @property
    def state(self):
        if self.do.getState():
            return PULL_TO_NONE
        else:
            return PULL_TO_GROUND

    def set(self, state):
        if state == PULL_TO_GROUND:
            self.do.setState(0)
        elif state == PULL_TO_NONE:
            self.do.setState(1)
        else:
            raise JigException("invalid SG input state {}".format(state))

class jigSPOutput(jigOutput):

    @property
    def state(self):
        relayState = self.do.getState()
        if relayState == 0:
            return PULL_TO_GROUND
        elif PullToBattery.state:
            return PULL_TO_BATTERY
        else:
            return PULL_TO_NONE

    def set(self, state):
        if state == PULL_TO_GROUND:
            self.do.setState(0)
        elif state == PULL_TO_NONE:
            if PullToBattery.state:
                raise JigException("cannot pull SP input to NONE with PullToBattery set")
            self.do.setState(1)
        elif state == PULL_TO_BATTERY:
            if not PullToBattery.state:
                raise JigException("cannot pull SP input to BATTERY with PullToBattery not set")
            self.do.setState(1)
        else:
            raise JigException("invalid SP input state {}".format(state))


class jigVoltage(object):
    '''voltage sensor'''

    def __init__(self, provider, channel):
        self.ai = VoltageInput()
        self.ai.setDeviceSerialNumber(provider)
        self.ai.setChannel(channel)
        self.ai.setOnAttachHandler(self._attached)
        self.ai.openWaitForAttachment(1000)

    def _attached(self, event):
        self.ai.setSensorType(VoltageSensorType.SENSOR_TYPE_1135)
        self.ai.setDataInterval(100)
        self.ai.setVoltageChangeTrigger(0)

    @property
    def voltage(self):
        return self.ai.getSensorValue()

    @property
    def unitInfo(self):
        return self.ai.getSensorUnit()


class jigCurrent(object):
    '''current sensor'''

    def __init__(self, provider, channel):
        self.ai = VoltageRatioInput()
        self.ai.setDeviceSerialNumber(provider)
        self.ai.setChannel(channel)
        self.ai.setOnAttachHandler(self._attached)
        self.ai.openWaitForAttachment(1000)

    def _attached(self, event):
        self.ai.setSensorType(VoltageRatioSensorType.SENSOR_TYPE_1122_DC)
        self.ai.setDataInterval(100)
        self.ai.setVoltageRatioChangeTrigger(0)

    @property
    def current(self):
        return self.ai.getSensorValue()

    @property
    def unitInfo(self):
        return self.ai.getSensorUnit()

SG0 = jigSGOutput(RelaySerial, 0, PULL_TO_GROUND)
SG1 = jigSGOutput(RelaySerial, 1, PULL_TO_GROUND)
SG2 = jigSGOutput(RelaySerial, 2, PULL_TO_GROUND)
SG3 = jigSGOutput(RelaySerial, 3, PULL_TO_GROUND)
SP1 = jigSPOutput(RelaySerial, 5, PULL_TO_GROUND)
SP2 = jigSPOutput(RelaySerial, 6, PULL_TO_GROUND)
SP3 = jigSPOutput(RelaySerial, 7, PULL_TO_GROUND)

Ignition = jigOutput(RelaySerial, 4, 0)
PullToBattery = jigOutput(MiscIOSerial, 0, 0)

Current = jigCurrent(MiscIOSerial, 0)
Voltage = jigVoltage(MiscIOSerial, 1)

# phidget22 bug requires sleep before trying to read values
# https://www.phidgets.com/phorum/viewtopic.php?t=8300
time.sleep(0.5)

def reset():
    SG0.reset()
    SG1.reset()
    SG2.reset()
    SG3.reset()
    SP1.reset()
    SP2.reset()
    SP3.reset()
    Ignition.reset()
    PullToBattery.reset()

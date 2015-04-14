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

import sys

from Phidgets.PhidgetException import PhidgetErrorCodes, PhidgetException
from Phidgets.Events.Events import AttachEventArgs, DetachEventArgs, ErrorEventArgs, InputChangeEventArgs, OutputChangeEventArgs, SensorChangeEventArgs
from Phidgets.Devices.InterfaceKit import InterfaceKit
from Phidgets.Phidget import PhidgetLogLevel

class JigException(Exception):
    pass

def errorHandler(e):
    try:
        source = e.device
        print("InterfaceKit %i: Phidget Error %i: %s" % (source.getSerialNum(), e.eCode, e.description))
    except PhidgetException as e:
        print("Phidget Exception %i: %s" % (e.code, e.details))

class jig(object):

    mio = None
    relays = None

    # Connect to the I/O board
    try:
        mio = InterfaceKit()
        mio.setOnErrorhandler(errorHandler)
        mio.openPhidget(MiscIOSerial)
    except PhidgetException as e:
        raise JigException("Phidget Exception %i connecting to MiscIO board: %s" % (e.code, e.details))

    try:            
        mio.waitForAttach(1000)
    except PhidgetException as e:
        raise JigException("Phidget Exception %i waiting for MiscIO board: %s" % (e.code, e.details))

    # Connect to the relay board
    try:
        relays = InterfaceKit()
        relays.setOnErrorhandler(errorHandler)
        relays.openPhidget(RelaySerial)
    except PhidgetException as e:
        raise JigException("Phidget Exception %i connecting to Relay board: %s" % (e.code, e.details))

    try:            
        relays.waitForAttach(1000)
    except PhidgetException as e:
        raise JigException("Phidget Exception %i waiting for Relay board: %s" % (e.code, e.details))


    def __init__(self):
        self.reset()

    def __del__(self):
        self.reset()

    def reset(self):
        '''pull all switch inputs to ground, turn off ignition and pull-to-battery'''
        self.setIgnition(False)
        self.setPullToBatt(PULL_TO_NONE)
        self.setSP(1, PULL_TO_GROUND)
        self.setSP(2, PULL_TO_GROUND)
        self.setSP(3, PULL_TO_GROUND)
        self.setSG(0, PULL_TO_GROUND)
        self.setSG(1, PULL_TO_GROUND)
        self.setSG(2, PULL_TO_GROUND)
        self.setSG(3, PULL_TO_GROUND)

    def setIgnition(self, state):
        '''enable or disable the ignition switch input to the master'''

        if state not in [True, False]:
            raise JigException("invalid ignition-enable state %s" % mode)

        jig.relays.setOutputState(4, state)

    def setPullToBatt(self, state):
        '''set the state of the PULL_TO_BATT / PULL_TO_NONE rail for the SP switches'''

        if state not in [PULL_TO_BATTERY, PULL_TO_NONE]:
            raise JigException("invalid pull-to-battery state %s" % mode)

        if jig.mio is not None:
            if state == PULL_TO_BATTERY:
                jig.mio.setOutputState(0, True)
            else:
                jig.mio.setOutputState(0, False)

    def setSP(self, index, mode):
        '''set the state of the SP1-3 input to the master'''

        if mode not in [PULL_TO_GROUND, PULL_TO_NONE, PULL_TO_BATTERY]:
            raise JigException("invalid SP pull mode %s" % mode)
        if index not in range(1, 4):
            raise JigException("invalid SP input index %s" % index)

        # actual state depends on setPullToBatt()
        if mode in (PULL_TO_BATTERY, PULL_TO_NONE):
            jig.relays.setOutputState(index + 4, True)
        else:
            jig.relays.setOutputState(index + 4, False)

    def setSG(self, index, mode):
        '''set the state of the SG0-3 input to the master'''

        if mode not in [PULL_TO_GROUND, PULL_TO_NONE]:
            raise JigException("invalid SG pull mode %s" % mode)
        if index not in range(0, 4):
            raise JigException("invalid SG input index %s" % index)

        if mode == PULL_TO_NONE:
            jig.relays.setOutputState(index, True)
        elif mode == PULL_TO_GROUND:
            jig.relays.setOutputState(index, False)
        else:
            raise JigException("PULL_TO_BATT not possible for SG0")

    @property
    def current():
        '''fetch the current sensor reading in Amperes'''
        return (jig.mio.getSensorValue(0) / 13.2) - 37.8787

    @property
    def voltage():
        '''fetch the voltage sensor reading in Volts'''
        return ((jig.mio.getSensorValue(1) / 200) - 2.5) / 0.0681


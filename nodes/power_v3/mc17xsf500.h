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

///@file mc17xsf500.h

#pragma once

#include <stdint.h>

namespace MC17XSF500
{
static const uint8_t num_channels = 5;

enum ControlRegister : uint8_t {
    kCtrlInit1 = 0x00,
    kCtrlInit2 = 0x10,
    kCtrlCH1Control = 0x20,
    kCtrlCH2Control = 0x30,
    kCtrlCH3Control = 0x40,
    kCtrlCH4Control = 0x50,
    kCtrlCH5Control = 0x60,
    kCtrlCH6Control = 0x70,
    kCtrlOutputControl = 0x80,
    kCtrlGlobalPWMControl = 0x90,
    kCtrlGlobalPWMValue = 0x94,
    kCtrlOvercurrentControl1 = 0xa0,
    kCtrlOvercurrentControl2 = 0xa4,
    kCtrlInputEnable = 0xb0,
    kCtrlPrescaler1 = 0xc0,
    kCtrlPrescaler2 = 0xc4,
    kCtrlOLControl = 0xd0,
    kCtrlOLLEDControl = 0xd4,
    kCtrlIncrementDecrement = 0xe0,
    kCtrlTestMode = 0xf0,

    kCtrlWDToggle = 0x08
};

enum StatusRegister : uint8_t {
    // readable registers
    kStatusQuick = 0x01,
    kStatusCH1 = 0x02,
    kStatusCH2 = 0x03,
    kStatusCH3 = 0x04,
    kStatusCH4 = 0x05,
    kStatusCH5 = 0x06,
    kStatusDevice = 0x07,
    kStatusIO = 0x08,
    kDeviceID = 0x09,
    kStatusTestMode = 0x0f
};

enum SyncEn : uint16_t {
    kSyncOff = 0,
    kSyncValid = 1,
    kSyncTrig0 = 2,
    kSyncTrig12 = 3
};

enum MuxSel : uint16_t {
    kMuxOff = 0,
    kMuxOut1 = 1,
    kMuxOut2 = 2,
    kMuxOut3 = 3,
    kMuxOut4 = 4,
    kMuxOut5 = 5,
    kMuxVPWR = 6,
    kMuxDieTemp = 7
};

enum NoHidSel : uint16_t {
    kNoHidAll = 0,
    kNoHid3 = 1,
    kNoHid34 = 2,
    kNoHidNone = 3
};

enum PhaseSel : uint16_t {
    kPhase0 = 0,
    kPhase90 = 1,
    kPhase180 = 2,
    kPhase270 = 3
};

enum PRSDivider : uint16_t {
    kPRSDiv4 = 0,
    kPRSDiv2 = 1,
    kPRSDiv1 = 2
};

enum Increment : uint16_t {
    kIncrementNone = 0,
    kIncrement4LSB = 1,
    kIncrement8LSB = 2,
    kIncrement16LSB = 3
};

struct Command {
    union {
        uint8_t     raw[2];
        uint16_t    val;

        // bitfields are all LSB first

        struct {
            uint16_t    data: 8;
            uint16_t    address: 8;
        }           address;

        struct {
            uint16_t    soa: 4;
            uint16_t    soa_mode: 1;
            uint16_t    mux_sel: 3;
            uint16_t    sync_en: 2;
            uint16_t    wdsel: 1;

            uint16_t    pad: 5;
        }           init_1;

        struct {
            uint16_t    otw_sel: 1;
            uint16_t    pwm_sync: 1;
            uint16_t    ochi_od: 5;
            uint16_t    no_hid: 2;
            uint16_t    ochi_transient: 1;
            uint16_t    ochi_thermal: 1;

            uint16_t    pad: 5;
        }           init_2;

        struct {
            uint16_t    pwm: 8;
            uint16_t    on: 1;
            uint16_t    phase_sel: 2;

            uint16_t    pad: 5;
        }           chX_control;

        struct {
            uint16_t    on: 6;
            uint16_t    psf: 5;

            uint16_t    pad: 5;
        }           output_control;

        struct {
            uint16_t    gpwm_enable: 6;

            uint16_t    pad: 10;
        }           global_pwm_enable;

        struct {
            uint16_t    pad: 8;

            uint16_t    gpwm: 8;
        }           global_pwm_value;

        struct {
            uint16_t    acm: 5;
            uint16_t    oclo: 5;

            uint16_t    pad: 6;
        }           over_current_control_1;

        struct {
            uint16_t    short_ochi: 5;
            uint16_t    no_ochi: 5;

            uint16_t    pad: 6;
        }           over_current_control_2;

        struct {
            uint16_t    inen1: 2;
            uint16_t    inen2: 2;
            uint16_t    inen3: 2;
            uint16_t    inen4: 2;

            uint16_t    pad: 8;
        }           input_enable;

        struct {
            uint16_t    prs1: 2;
            uint16_t    prs2: 2;
            uint16_t    prs3: 2;
            uint16_t    prs4: 2;
            uint16_t    prs5: 2;

            uint16_t    pad: 6;
        }           prescaler_1;

        struct {
            uint16_t    prs6: 2;

            uint16_t    pad: 14;
        }           prescaler_2;

        struct {
            uint16_t    oloff_en: 5;
            uint16_t    olon_dgl: 5;

            uint16_t    pad: 6;
        }           ol_control;

        struct {
            uint16_t    olled_en: 5;
            uint16_t    olled_trg: 1;

            uint16_t    pad: 10;
        }           olled_control;

        struct {
            uint16_t    incr1: 2;
            uint16_t    incr2: 2;
            uint16_t    incr3: 2;
            uint16_t    incr4: 2;
            uint16_t    incr5: 2;
            uint16_t    incr_sgn: 1;

            uint16_t    pad: 5;
        }           increment_decrement;
    };
};

enum OCStatus : uint16_t {
    kOCNone = 0,
    kOCHI1 = 1,
    kOCHI2 = 2,
    kOCHI3 = 3,
    kOCLO = 4,
    kOCHIOD = 5,
    kOCSSC = 6
};

enum DeviceType : uint16_t {
    kDTPenta32 = 0,
    kDTPenta05 = 1,
    kDTQuad22 = 2,
    kDTQuad04 = 3,
    kDTTriple12 = 4,
    kDTTriple03 = 5
};

union Status {
    uint8_t     raw[2];
    uint16_t    val;

    struct {
        uint16_t    data: 12;
        uint16_t    reg: 4;

    }           address;

    struct {
        uint16_t    qsf: 5;
        uint16_t    clkf: 1;
        uint16_t    rcf: 1;
        uint16_t    cpf: 1;
        uint16_t    olf: 1;
        uint16_t    ovlf: 1;
        uint16_t    dsf: 1;
        uint16_t    fm: 1;

        uint16_t    reg: 4;

    }           quick_status;

    struct {
        uint16_t    oloff: 1;
        uint16_t    olon: 1;
        uint16_t    oc_status: 3;
        uint16_t    otw: 1;
        uint16_t    ots: 1;
        uint16_t    pad2: 1;
        uint16_t    olf: 1;
        uint16_t    ovlf: 1;
        uint16_t    dsf: 1;
        uint16_t    fm: 1;

        uint16_t    reg: 4;

    }           chX_status;

    struct {
        uint16_t    limp: 1;
        uint16_t    spif: 1;
        uint16_t    uvf: 1;
        uint16_t    ovf: 1;
        uint16_t    tmf: 1;
        uint16_t    pad2: 3;
        uint16_t    olf: 1;
        uint16_t    ovlf: 1;
        uint16_t    dsf: 1;
        uint16_t    fm: 1;

        uint16_t    reg: 4;

    }           device_status;

    struct {
        uint16_t    out: 5;
        uint16_t    in: 4;
        uint16_t    toggle: 1;
        uint16_t    pad2: 1;
        uint16_t    fm: 1;

        uint16_t    reg: 4;

    }           IO_status;

    struct {
        uint16_t    device_type: 3;
        uint16_t    device_family: 2;
        uint16_t    design_status: 3;
        uint16_t    pad2: 2;
        uint16_t    uvf: 1;
        uint16_t    fm: 1;

        uint16_t    reg: 4;

    }           device_id;
};

extern void configure();
extern void set(uint8_t channel, uint8_t state);
extern Status get_status(uint8_t reg);

} // namespace MC17XSF500

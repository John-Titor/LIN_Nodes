#pragma once

// v1 master using the generic node board
//

#define pinP0          portB[0]
#define pinP1          portB[1]
#define pinP2          portB[2]
#define pinP3          portB[3]
#define pinP4          portB[4]
#define pinP5          portB[5]
#define pinP6          portB[6]
#define pinP7          portA[7]

#define pinMode1       portA[4]
#define pinMode2       portA[2]
#define pinMode4       portA[5]
#define pinMode8       portA[3]

#define pinMOSI        portA[4]
#define pinMISO        portA[2]
#define pinSCK         portA[5]
#define pinCS          portB[1]

#define pinSDA         portB[0]
#define pinSCL         portB[2]

#define pinLINRX       portA[0]
#define pinLINTX       portA[1]
#define pinLINCS       portA[6]

#define pinADC7        portA[7]
#define pinADC8        portB[5]
#define pinADC9        portB[6]

#define pinINT0        portB[6]

#define pinDebugTX     portA[7]

#define paramSPAssign(_n)          (LIN::SwitchID)Parameter(_n)
#define paramSGAssign(_n)          (LIN::SwitchID)Parameter(8 + _n)
#define paramTurnBlinkPeriod       Parameter(22)
#define paramPassingBlinkCount     Parameter(23)
#define paramPathLightPeriod       Parameter(24)
#define paramInteriorLightPeriod   Parameter(25)
#define paramWelcomeLightPeriod    Parameter(26)
#define paramBrakeBlinkPeriod      Parameter(27)
#define paramBrakeBlinkCount       Parameter(28)


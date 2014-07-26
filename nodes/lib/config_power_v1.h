/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <msmith@purgatory.org> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma once

//
// v1 power node
//

static const uint8_t kBoardFunctionID = 1;

#define pinOUT1        portB[0]
#define pinSTATUS1     portB[1]
#define pinOUT2        portB[2]
#define pinSTATUS2     portB[3]
#define pinOUT3        portB[4]
#define pinSTATUS3     portB[5]
#define pinOUT4        portB[6]
#define pinSTATUS4     portA[7]

#define pinMode1       portA[4]
#define pinMode2       portA[2]
#define pinMode4       portA[5]
#define pinMode8       portA[3]

#define pinLINRX       portA[0]
#define pinLINTX       portA[1]
#define pinLINCS       portA[6]

#define pinDebugTX     portA[7]

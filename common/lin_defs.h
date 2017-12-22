// AUTOMATICALLY GENERATED - DO NOT EDIT
#pragma once
#ifdef __AVR__
# include <avr/pgmspace.h>
#else
# define PROGMEM
#endif
#include "lin_types.h"


namespace Encoding
{
extern bool                 invalid(uint8_t encoding, uint16_t value);
#ifdef LIN_DEFS_WITH_STRINGS
extern const PROGMEM char   *name(uint8_t encoding);
extern const PROGMEM char   *info(uint8_t encoding, uint16_t value);
extern bool                 value(uint8_t encoding, const char *info, uint16_t &value);
#endif // LIN_DEFS_WITH_STRINGS
} // namespace Encoding

static const uint8_t kEncoding_board_function = 0;
static const uint8_t kEncoding_operation_magic = 1;
static const uint8_t kEncoding_service_error = 2;
static const uint8_t kEncoding_bl_status = 3;
static const uint8_t kEncoding_bl_reason = 4;
static const uint8_t kEncoding_input_assignment = 5;
static const uint8_t kEncoding_sp_input_type = 6;
static const uint8_t kEncoding_sg_switch_input_type = 7;
static const uint8_t kEncoding_sg_mixed_input_type = 8;
static const uint8_t kEncoding_msec = 9;
static const uint8_t kEncoding_sec = 10;
static const uint8_t kEncoding_up_to_10 = 11;
static const uint8_t kEncoding_node_address = 12;
static const uint8_t kEncoding_pci = 13;
static const uint8_t kEncoding_service_id = 14;
static const uint8_t kEncoding_v3_device_status = 15;
static const uint8_t kEncoding_v3_output_status = 16;
static const uint8_t kEncoding_v3_output_type = 17;
static const uint8_t kEncoding_v3_output_assignment = 18;
static const uint8_t kEncoding_RPM = 19;
static const uint8_t kEncoding_PSI = 20;
static const uint8_t kEncoding_F = 21;
static const uint8_t kEncoding_V = 22;
static const uint8_t kEncoding_lambda = 23;
static const uint8_t kEncoding_MPH = 24;
static const uint8_t kEncoding_none = 0xff;

namespace board_function
{
static const uint16_t kUnknown = 0;
static const uint16_t kMaster = 1;
static const uint16_t kPowerV1 = 2;
static const uint16_t kPowerV3 = 3;
static const uint16_t kECUBridge = 4;
static const uint16_t kDashboard = 5;
static const uint16_t kTester = 6;
static const uint16_t kUnconfigured = 255;
static const uint16_t kNumEncodings = 8;
} // namespace board_function

namespace operation_magic
{
static const uint16_t kProgram = 0;
static const uint16_t kBootloader = 1;
static const uint16_t kTest = 2;
static const uint16_t kEnterBootloader = 0x4f42;
static const uint16_t kSetDefaults = 0x4644;
static const uint16_t kNumEncodings = 5;
} // namespace operation_magic

namespace service_error
{
static const uint16_t kFunctionNotSupported = 0x12;
static const uint16_t kIncorrectLength = 0x13;
static const uint16_t kConditionsNotCorrect = 0x22;
static const uint16_t kOutOfRange = 0x31;
static const uint16_t kAccessDenied = 0x33;
static const uint16_t kGeneralFailure = 0x72;
static const uint16_t kNumEncodings = 6;
} // namespace service_error

namespace bl_status
{
static const uint16_t kWaitingForProgrammer = 0;
static const uint16_t kReadyForPage = 1;
static const uint16_t kPageInProgress = 2;
static const uint16_t kPageCRCError = 3;
static const uint16_t kPageAddressError = 4;
static const uint16_t kNumEncodings = 5;
} // namespace bl_status

namespace bl_reason
{
static const uint16_t kUnspecified = 0;
static const uint16_t kCRCMismatch = 1;
static const uint16_t kForced = 2;
static const uint16_t kNumEncodings = 3;
} // namespace bl_reason

namespace input_assignment
{
static const uint16_t kUnassigned = 0;
static const uint16_t kIgnition = 1;
static const uint16_t kStart = 2;
static const uint16_t kMarkerLights = 3;
static const uint16_t kHeadLights = 4;
static const uint16_t kHighBeam = 5;
static const uint16_t kHighBeamToggle = 6;
static const uint16_t kFogLight = 7;
static const uint16_t kLeftTurn = 8;
static const uint16_t kRightTurn = 9;
static const uint16_t kBrake = 10;
static const uint16_t kReverse = 11;
static const uint16_t kDoor = 12;
static const uint16_t kInteriorLight = 13;
static const uint16_t kHazard = 14;
static const uint16_t kDoorUnlock = 15;
static const uint16_t kLightsUp = 16;
static const uint16_t kCabinFan1 = 17;
static const uint16_t kCabinFan2 = 18;
static const uint16_t kCabinFan3 = 19;
static const uint16_t kWiperInt = 20;
static const uint16_t kWiperLow = 21;
static const uint16_t kWiperHigh = 22;
static const uint16_t kRearDefrost = 23;
static const uint16_t kNumEncodings = 24;
} // namespace input_assignment

namespace sp_input_type
{
static const uint16_t kSwitchToGround = 0;
static const uint16_t kSwitchToBattery = 1;
static const uint16_t kLogic = 2;
static const uint16_t kInvertedLogic = 3;
static const uint16_t kNumEncodings = 4;
} // namespace sp_input_type

namespace sg_switch_input_type
{
static const uint16_t kSwitchToGround = 0;
static const uint16_t kLogic = 2;
static const uint16_t kNumEncodings = 2;
} // namespace sg_switch_input_type

namespace sg_mixed_input_type
{
static const uint16_t kSwitchToGround = 0;
static const uint16_t kLogic = 2;
static const uint16_t kAnalogNoBias = 4;
static const uint16_t kAnalog2mABias = 5;
static const uint16_t kAnalog16mABias = 6;
static const uint16_t kNumEncodings = 5;
} // namespace sg_mixed_input_type

namespace msec
{
static const uint16_t kDisabled = 0;
static const uint16_t kMillisecondsMin = 100;
static const uint16_t kMillisecondsMax = 65000;
static const uint16_t kNumEncodings = 2;
} // namespace msec

namespace sec
{
static const uint16_t kDisabled = 0;
static const uint16_t kSecondsMin = 1;
static const uint16_t kSecondsMax = 600;
static const uint16_t kNumEncodings = 2;
} // namespace sec

namespace up_to_10
{
static const uint16_t kDisabled = 0;
static const uint16_t kCountMin = 1;
static const uint16_t kCountMax = 10;
static const uint16_t kNumEncodings = 2;
} // namespace up_to_10

namespace node_address
{
static const uint16_t kSleep = 0;
static const uint16_t kNodeAddressMin = 1;
static const uint16_t kNodeAddressMax = 125;
static const uint16_t kFunctional = 126;
static const uint16_t kBroadcast = 127;
static const uint16_t kNumEncodings = 4;
} // namespace node_address

namespace pci
{
static const uint16_t kSingleFrame = 0;
static const uint16_t kFirstFrame = 1;
static const uint16_t kContinuationFrame = 2;
static const uint16_t kNumEncodings = 3;
} // namespace pci

namespace service_id
{
static const uint16_t kReadDataByID = 0x22;
static const uint16_t kWriteDataByID = 0x2e;
static const uint16_t kTesterPresent = 0x3e;
static const uint16_t kReadByID = 0xb2;
static const uint16_t kDataDump = 0xb4;
static const uint16_t kErrorResponse = 0x7f;
static const uint16_t kResponseOffset = 0x40;
static const uint16_t kNumEncodings = 7;
} // namespace service_id

namespace v3_device_status
{
static const uint16_t kOK = 0;
static const uint16_t kDeviceFailure = 1;
static const uint16_t kOvervoltage = 2;
static const uint16_t kUndervoltage = 3;
static const uint16_t kOverload = 4;
static const uint16_t kNumEncodings = 5;
} // namespace v3_device_status

namespace v3_output_status
{
static const uint16_t kOK = 0;
static const uint16_t kUnassigned = 1;
static const uint16_t kOpenLoad = 2;
static const uint16_t kTemperatureWarning = 3;
static const uint16_t kTemperatureShutdown = 4;
static const uint16_t kOverCurrent = 5;
static const uint16_t kNumEncodings = 6;
} // namespace v3_output_status

namespace v3_output_type
{
static const uint16_t k5AGeneric = 0;
static const uint16_t k10AGeneric = 1;
static const uint16_t kLED = 2;
static const uint16_t kHID = 3;
static const uint16_t kLowPowerBulb = 4;
static const uint16_t kHighPowerBulb = 5;
static const uint16_t kMotor = 6;
static const uint16_t kNumEncodings = 7;
} // namespace v3_output_type

namespace v3_output_assignment
{
static const uint16_t kUnassigned = 0;
static const uint16_t kIgnition = 1;
static const uint16_t kStart = 2;
static const uint16_t kLightsUp = 3;
static const uint16_t kLightsDown = 4;
static const uint16_t kHeadLights = 5;
static const uint16_t kLowBeam = 6;
static const uint16_t kHighBeam = 7;
static const uint16_t kFogLights = 8;
static const uint16_t kMarkerLights = 9;
static const uint16_t kLeftTurn = 10;
static const uint16_t kLeftTurnMarker = 11;
static const uint16_t kRightTurn = 12;
static const uint16_t kRightTurnMarker = 13;
static const uint16_t kBrake = 14;
static const uint16_t kReverse = 15;
static const uint16_t kInteriorLight = 16;
static const uint16_t kCabinFan1 = 17;
static const uint16_t kCabinFan2 = 18;
static const uint16_t kCabinFan3 = 19;
static const uint16_t kCabinFan4 = 20;
static const uint16_t kWiperLow = 21;
static const uint16_t kWiperHigh = 22;
static const uint16_t kRearDefrost = 23;
static const uint16_t kNumEncodings = 24;
} // namespace v3_output_assignment

namespace RPM
{
static const uint16_t kRPMMin = 0;
static const uint16_t kRPMMax = 255;
static const uint16_t kNumEncodings = 1;
} // namespace RPM

namespace PSI
{
static const uint16_t kPSIMin = 0;
static const uint16_t kPSIMax = 255;
static const uint16_t kNumEncodings = 1;
} // namespace PSI

namespace F
{
static const uint16_t kTemperatureMin = 0;
static const uint16_t kTemperatureMax = 255;
static const uint16_t kNumEncodings = 1;
} // namespace F

namespace V
{
static const uint16_t kVoltsMin = 0;
static const uint16_t kVoltsMax = 255;
static const uint16_t kNumEncodings = 1;
} // namespace V

namespace lambda
{
static const uint16_t kLambdaMin = 0;
static const uint16_t kLambdaMax = 255;
static const uint16_t kNumEncodings = 1;
} // namespace lambda

namespace MPH
{
static const uint16_t kMPHMin = 0;
static const uint16_t kMPHMax = 255;
static const uint16_t kNumEncodings = 1;
} // namespace MPH

namespace Frame
{
#ifdef LIN_DEFS_WITH_STRINGS
extern const PROGMEM char *name(uint8_t fid);
#endif
}

static const uint8_t kFrameIDSlaveResponse = 0x3d;
static const uint8_t kFrameIDRelays = 0x01;
static const uint8_t kFrameIDMasterRequest = 0x3c;
static const uint8_t kFrameIDECUData = 0x02;

struct Response
{
    Response() : _bytes {0, 0, 0, 0, 0, 0, 0, 0} {}
    union
    {
        uint8_t   _bytes[8];
        uint64_t  _raw;
        struct {
            uint64_t  nad:8;
            uint64_t  length:4;
            uint64_t  pci:4;
            uint64_t  sid:8;
            uint64_t  d1:8;
            uint64_t  d2:8;
            uint64_t  d3:8;
            uint64_t  d4:8;
            uint64_t  d5:8;
        } SlaveResponse;
        struct {
            uint64_t  nad:8;
            uint64_t  length:4;
            uint64_t  pci:4;
            uint64_t  sid:8;
            uint64_t  vendor:16;
            uint64_t  function:16;
            uint64_t  variant:8;
        } ReadByID;
        struct {
            uint64_t  nad:8;
            uint64_t  length:4;
            uint64_t  pci:4;
            uint64_t  sid:8;
            uint64_t  index:16;
            uint64_t  value:16;
            uint64_t  d5:8;
        } DataByID;
        struct {
            uint64_t  nad:8;
            uint64_t  length:4;
            uint64_t  pci:4;
            uint64_t  sid:8;
            uint64_t  original_sid:8;
            uint64_t  error:8;
            uint64_t  d3:8;
            uint64_t  d4:8;
            uint64_t  d5:8;
        } ServiceError;
        struct {
            uint64_t  Ignition:1;
            uint64_t  Start:1;
            uint64_t  LightsUp:1;
            uint64_t  LightsDown:1;
            uint64_t  HeadLights:1;
            uint64_t  LowBeam:1;
            uint64_t  HighBeam:1;
            uint64_t  FogLights:1;
            uint64_t  MarkerLights:1;
            uint64_t  LeftTurn:1;
            uint64_t  LeftTurnMarker:1;
            uint64_t  RightTurn:1;
            uint64_t  RightTurnMarker:1;
            uint64_t  Brake:1;
            uint64_t  Reverse:1;
            uint64_t  InteriorLight:1;
            uint64_t  CabinFan1:1;
            uint64_t  CabinFan2:1;
            uint64_t  CabinFan3:1;
            uint64_t  CabinFan4:1;
            uint64_t  WiperLow:1;
            uint64_t  WiperHigh:1;
            uint64_t  RearDefrost:1;
        } Relays;
        struct {
            uint64_t  nad:8;
            uint64_t  length:4;
            uint64_t  pci:4;
            uint64_t  sid:8;
            uint64_t  d1:8;
            uint64_t  d2:8;
            uint64_t  d3:8;
            uint64_t  d4:8;
            uint64_t  d5:8;
        } MasterRequest;
        struct {
            uint64_t  RPM:8;
            uint64_t  oilPressure:8;
            uint64_t  oilTemperature:8;
            uint64_t  coolantTemperature:8;
            uint64_t  fuelPressure:8;
            uint64_t  voltage:8;
            uint64_t  AFR:8;
            uint64_t  roadSpeed:8;
        } ECUData;
    };
};

namespace Generic
{
extern bool param_exists(Parameter::Address address);
extern uint16_t param_default(Parameter::Address address);
extern uint8_t param_encoding(Parameter::Address address);
#ifdef LIN_DEFS_WITH_STRINGS
extern const PROGMEM char *param_name(Parameter::Address address);
#endif // LIN_DEFS_WITH_STRINGS

static const uint16_t kParamProtocolVersion = 0x0000;
static const uint16_t kParamBoardFunction = 0x0001;
static const uint16_t kParamOperationMode = 0x0002;
static const uint16_t kParamFirmwareVersion = 0x0003;
static const uint16_t kParamFirmwarePageSize = 0x0004;
static const uint16_t kParamWatchdogResets = 0x0100;
static const uint16_t kParamSupplyVoltage = 0x0101;
static const uint16_t kParamTemperature = 0x0102;
static const uint16_t kParamFreeMem = 0x0103;
static const uint16_t kParamLine = 0x0200;
static const uint16_t kParamChecksum = 0x0201;
static const uint16_t kParamParity = 0x0202;
static const uint16_t kParamFraming = 0x0203;
static const uint16_t kParamSynch = 0x0204;
static const uint16_t kParamProtocol = 0x0205;

} // namespace Generic

namespace Bootloader
{
extern bool param_exists(Parameter::Address address);
extern uint16_t param_default(Parameter::Address address);
extern uint8_t param_encoding(Parameter::Address address);
#ifdef LIN_DEFS_WITH_STRINGS
extern const PROGMEM char *param_name(Parameter::Address address);
#endif // LIN_DEFS_WITH_STRINGS

static const uint16_t kParamStatus = 0x0300;
static const uint16_t kParamPageAddress = 0x0301;
static const uint16_t kParamPageOffset = 0x0302;
static const uint16_t kParamPageCRC = 0x0303;
static const uint16_t kParamDebugPointer = 0x0304;
static const uint16_t kParamReason = 0x0305;
static const uint16_t kParamMemory = 0x0306;
static const uint16_t kParamEEPROM = 0x0307;

} // namespace Bootloader

namespace Master
{
extern bool param_exists(Parameter::Address address);
extern uint16_t param_default(Parameter::Address address);
extern uint8_t param_encoding(Parameter::Address address);
#ifdef LIN_DEFS_WITH_STRINGS
extern const PROGMEM char *param_name(Parameter::Address address);
#endif // LIN_DEFS_WITH_STRINGS

static const uint16_t kParamSP1Assign = 0x0400;
static const uint16_t kParamSP1Mode = 0x0401;
static const uint16_t kParamSP2Assign = 0x0402;
static const uint16_t kParamSP2Mode = 0x0403;
static const uint16_t kParamSP3Assign = 0x0404;
static const uint16_t kParamSP3Mode = 0x0405;
static const uint16_t kParamSP4Assign = 0x0406;
static const uint16_t kParamSP4Mode = 0x0407;
static const uint16_t kParamSP5Assign = 0x0408;
static const uint16_t kParamSP5Mode = 0x0409;
static const uint16_t kParamSP6Assign = 0x040a;
static const uint16_t kParamSP6Mode = 0x040b;
static const uint16_t kParamSP7Assign = 0x040c;
static const uint16_t kParamSP7Mode = 0x040d;
static const uint16_t kParamSG0Assign = 0x040e;
static const uint16_t kParamSG0Mode = 0x040f;
static const uint16_t kParamSG1Assign = 0x0410;
static const uint16_t kParamSG1Mode = 0x0411;
static const uint16_t kParamSG2Assign = 0x0412;
static const uint16_t kParamSG2Mode = 0x0413;
static const uint16_t kParamSG3Assign = 0x0414;
static const uint16_t kParamSG3Mode = 0x0415;
static const uint16_t kParamSG4Assign = 0x0416;
static const uint16_t kParamSG4Mode = 0x0417;
static const uint16_t kParamSG5Assign = 0x0418;
static const uint16_t kParamSG5Mode = 0x0419;
static const uint16_t kParamSG6Assign = 0x041a;
static const uint16_t kParamSG6Mode = 0x041b;
static const uint16_t kParamSG7Assign = 0x041c;
static const uint16_t kParamSG7Mode = 0x041d;
static const uint16_t kParamSG8Assign = 0x041e;
static const uint16_t kParamSG8Mode = 0x041f;
static const uint16_t kParamSG9Assign = 0x0420;
static const uint16_t kParamSG9Mode = 0x0421;
static const uint16_t kParamSG10Assign = 0x0422;
static const uint16_t kParamSG10Mode = 0x0423;
static const uint16_t kParamSG11Assign = 0x0424;
static const uint16_t kParamSG11Mode = 0x0425;
static const uint16_t kParamSG12Assign = 0x0426;
static const uint16_t kParamSG12Mode = 0x0427;
static const uint16_t kParamSG13Assign = 0x0428;
static const uint16_t kParamSG13Mode = 0x0429;
static const uint16_t kParamTurnBlinkPeriod = 0x042a;
static const uint16_t kParamPassingBlinkCount = 0x042b;
static const uint16_t kParamPathLightTime = 0x042c;
static const uint16_t kParamInteriorLightTime = 0x042d;
static const uint16_t kParamWelcomeLightTime = 0x042e;
static const uint16_t kParamWiperInterval = 0x042f;

} // namespace Master

namespace PowerV3
{
extern bool param_exists(Parameter::Address address);
extern uint16_t param_default(Parameter::Address address);
extern uint8_t param_encoding(Parameter::Address address);
#ifdef LIN_DEFS_WITH_STRINGS
extern const PROGMEM char *param_name(Parameter::Address address);
#endif // LIN_DEFS_WITH_STRINGS

static const uint16_t kParamDeviceStatus = 0x0300;
static const uint16_t kParamCH1Status = 0x0301;
static const uint16_t kParamCH1Current = 0x0302;
static const uint16_t kParamCH2Status = 0x0303;
static const uint16_t kParamCH2Current = 0x0304;
static const uint16_t kParamCH3Status = 0x0305;
static const uint16_t kParamCH3Current = 0x0306;
static const uint16_t kParamCH4Status = 0x0307;
static const uint16_t kParamCH4Current = 0x0308;
static const uint16_t kParamCH5Status = 0x0309;
static const uint16_t kParamCH5Current = 0x030a;
static const uint16_t kParamInfo0 = 0x030b;
static const uint16_t kParamInfo1 = 0x030c;
static const uint16_t kParamInfo2 = 0x030d;
static const uint16_t kParamInfo3 = 0x030e;
static const uint16_t kParamInfo4 = 0x030f;
static const uint16_t kParamInfo5 = 0x0310;
static const uint16_t kParamInfo6 = 0x0311;
static const uint16_t kParamInfo7 = 0x0312;
static const uint16_t kParamCH1Type = 0x0400;
static const uint16_t kParamCH2Type = 0x0401;
static const uint16_t kParamCH3Type = 0x0402;
static const uint16_t kParamCH4Type = 0x0403;
static const uint16_t kParamCH5Type = 0x0404;
static const uint16_t kParamCH1Assign = 0x0405;
static const uint16_t kParamCH2Assign = 0x0406;
static const uint16_t kParamCH3Assign = 0x0407;
static const uint16_t kParamCH4Assign = 0x0408;
static const uint16_t kParamCH5Assign = 0x0409;

} // namespace PowerV3

namespace Generic
{
    static const uint8_t kNodeAddress = 1;
    static const uint16_t kNodeSupplier = 0xb007;
    static const uint16_t kNodeFunction = 0x0000;
    static const uint8_t kNodeVariant = 0x00;
    static const uint16_t kNodeProtocolVersion = ((uint16_t)2 << 8) | 1;
} // namespace Generic

namespace Bootloader
{
    static const uint8_t kNodeAddress = 32;
    static const uint16_t kNodeSupplier = 0xb007;
    static const uint16_t kNodeFunction = 0x0000;
    static const uint8_t kNodeVariant = 0x00;
} // namespace Bootloader

namespace Master
{
    static const uint8_t kNodeAddress = 1;
    static const uint16_t kNodeSupplier = 0xb007;
    static const uint16_t kNodeFunction = 0x0001;
    static const uint8_t kNodeVariant = 0x01;
    static const uint16_t kNodeProtocolVersion = ((uint16_t)1 << 8) | 0;
} // namespace Master

namespace PowerV3
{
    static const uint8_t kNodeAddress = 2;
    static const uint16_t kNodeSupplier = 0xb007;
    static const uint16_t kNodeFunction = 0x0003;
    static const uint8_t kNodeVariant = 0x01;
} // namespace PowerV3

namespace ECUBridge
{
    static const uint8_t kNodeAddress = 18;
    static const uint16_t kNodeSupplier = 0xb007;
    static const uint16_t kNodeFunction = 0x0004;
    static const uint8_t kNodeVariant = 0x10;
} // namespace ECUBridge

namespace Dashboard
{
    static const uint8_t kNodeAddress = 19;
    static const uint16_t kNodeSupplier = 0xb007;
    static const uint16_t kNodeFunction = 0x0005;
    static const uint8_t kNodeVariant = 0x01;
} // namespace Dashboard

namespace Tester
{
    static const uint8_t kNodeAddress = 20;
    static const uint16_t kNodeSupplier = 0xb007;
    static const uint16_t kNodeFunction = 0x0006;
    static const uint8_t kNodeVariant = 0x01;
    static const uint16_t kNodeProtocolVersion = ((uint16_t)1 << 8) | 0;
} // namespace Tester

static const uint8_t kNodeAddressMax = 32;

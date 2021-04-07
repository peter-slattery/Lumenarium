//
// File: blumen_lumen.h
// Author: Peter Slattery
// Creation Date: 2021-01-15
//
#ifndef BLUMEN_LUMEN_H

#include "message_queue.h"

enum bl_pattern_mode
{
    BlumenPattern_Standard,
    BlumenPattern_VoiceCommand,
    
    BlumenPattern_Count,
};

enum bl_python_packet_type
{
    PacketType_Invalid = 0,
    PacketType_PatternCommand = 1,
    PacketType_MotorState = 2,
    PacketType_Temperature = 3,
    PacketType_LumenariumStatus = 4,
};

enum bl_motor_state_value
{
    MotorState_Invalid = 0,
    
    MotorState_Closed = 1,
    MotorState_Open = 2,
    MotorState_HalfOpen = 3,
    MotorState_MostlyOpen = 4,
};

#pragma pack(push, 1)
typedef struct motor_packet
{
    u8 FlowerPositions[3];
} motor_packet;

typedef struct motor_status_packet
{
    motor_packet Pos;
    /*
u8 Motor1Pos;
u8 Motor2Pos;
u8 Motor3Pos;
*/
    u8 MotorStatus[3];
    u16 Temperature;
    
} motor_status_packet;

typedef struct microphone_packet
{
    b8 ChangeAnimation;
    char AnimationFileName[32];
    b8 SetLayer;
    char LayerName[32];
    r32 LayerOpacity;
    b8 SetLayerParamColor;
    char LayerParamColor[7];
    r32 OverrideDuration;
} microphone_packet;

typedef struct temp_packet
{
    s8 Temperature;
} temp_packet;

enum motor_event_type
{
    MotorEvent_Close = 0,
    MotorEvent_Open = 1,
};

typedef struct status_packet
{
    u8 NextMotorEventType;
    // u16 Padding;
    u32 NextEventTime;
    
    char AnimFileName[32];
} status_packet;

typedef struct blumen_packet
{
    u8 Type;
    union
    {
        motor_packet MotorPacket;
        motor_status_packet MotorStatusPacket;
        microphone_packet MicPacket;
        temp_packet TempPacket;
        status_packet StatusPacket;
    };
} blumen_packet;

#pragma pack(pop)

// TODO(pjs): Refactor this -> blumen_network_job_state
struct mic_listen_job_data
{
    bool* Running;
    
    platform_socket_manager* SocketManager;
    blumen_network_msg_queue* IncomingMsgQueue;
    
    blumen_network_msg_queue* OutgoingMsgQueue;
    
    // Status
    bool IsConnected;
};

typedef struct time_range
{
    s32 StartHour;
    s32 StartMinute;
    
    s32 EndHour;
    s32 EndMinute;
} time_range;

internal bool
SystemTimeIsInTimeRange(system_time SysTime, time_range Range)
{
    bool Result = (SysTime.Hour >= Range.StartHour &&
                   SysTime.Minute >= Range.StartMinute &&
                   SysTime.Hour <= Range.EndHour &&
                   SysTime.Minute <= Range.EndMinute);
    return Result;
}

#include "blumen_lumen_settings.h"

struct blumen_lumen_state
{
    bool Running;
    
    blumen_network_msg_queue IncomingMsgQueue;
    blumen_network_msg_queue OutgoingMsgQueue;
    
    temp_job_req JobReq;
    
    platform_thread_handle MicListenThread;
    mic_listen_job_data MicListenJobData;
    
    motor_packet LastKnownMotorState;
    u64 LastTimeMotorStateChanged[BL_FLOWER_COUNT];
    b8 ShouldDimUpperLeds[BL_FLOWER_COUNT];
    
    // NOTE(pjs): Based on temperature data from weatherman
    // dim the leds.
    r32 BrightnessPercent;
    s8 LastTemperatureReceived;
    system_time LastStatusUpdateTime;
    
    system_time LastSendTime;
    
    phrase_hue StandardPatternHues;
    phrase_hue AssemblyColors[BL_FLOWER_COUNT];
    u32 LastAssemblyColorSet;
    
    // The indices of this array are the index the clear core uses to 
    // represent a motor. 
    // The values of the array are the names Lumenarium uses to 
    // represent assemblies.
    // 
    u32 AssemblyNameToClearCoreMapCount;
    u64* AssemblyNameToClearCore_Names;
    
    bl_pattern_mode PatternMode;
    animation_handle_array ModeAnimations[BlumenPattern_Count];
    
    phrase_hue_map PhraseHueMap;
    
    bool InPhraseReceptionMode;
    phrase_hue NextHotHue;
    system_time TimePhraseReceptionBegan;
    system_time TimeLastPhraseReceived;
    
    system_time TimeLastSetToVoiceMode;
    phrase_hue LastHuePhrase;
    
    r32 PatternSpeed;
    
    // Debug
    motor_packet DEBUG_PendingMotorPacket;
    bool DEBUG_IgnoreWeatherDimmingLeds;
    
    bool ShouldUpdateLog;
};

#include "message_queue.cpp"

internal phrase_hue
BlumenLumen_GetCurrentHue(blumen_lumen_state* BLState, assembly Assembly)
{
    phrase_hue Result = {};
    
    switch (BLState->PatternMode)
    {
        case BlumenPattern_Standard:
        {
            Result = BLState->StandardPatternHues;
        }break;
        
        case BlumenPattern_VoiceCommand:
        {
            Result = BLState->AssemblyColors[Assembly.AssemblyIndex % 3];
        }break;
    }
    
    return Result;
}
















// If you change anything, exit lumenarium if its running
// then in this application hit f1 to compile then
// go to remedybg (the debugger) and hit f5


// don't touch this
u8 LastPosition = 1;

u8 ClosedValue = 1;
u8 OpenValue = 2;


r64 MotorTimeElapsed = 0;
r64 OpenClosePeriod = 15.0f;


























#define BLUMEN_LUMEN_H
#endif // BLUMEN_LUMEN_H
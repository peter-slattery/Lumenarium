//
// File: blumen_lumen.h
// Author: Peter Slattery
// Creation Date: 2021-01-15
//
#ifndef BLUMEN_LUMEN_H

#include "message_queue.h"

enum bl_debug_ui_mode
{
    BlumenDebug_Motors,
    BlumenDebug_Leds,
    BlumenDebug_Awaken,
    
    BlumenDebug_Count,
};

char* BlDebugUiModeStrings[] = {
    "Motors",
    "Leds",
    "Awaken",
};

enum bl_pattern_mode
{
    BlumenPattern_Standard,
    BlumenPattern_VoiceCommand,
    BlumenPattern_NoControl,
    
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
SystemTimeIsBeforeTime(system_time SysTime, s32 Hour, s32 Minute)
{
    bool Result = false;
    if (SysTime.Hour == Hour) {
        Result = SysTime.Minute < Minute;
    } else {
        Result = SysTime.Hour < Hour;
    }
    return Result;
}

internal bool
SystemTimeIsAfterTime(system_time SysTime, s32 Hour, s32 Minute)
{
    bool Result = false;
    if (SysTime.Hour == Hour) {
        Result = SysTime.Minute >= Minute;
    } else {
        Result = SysTime.Hour > Hour;
    }
    return Result;
}

internal bool
SystemTimeIsInTimeRange(system_time SysTime, time_range Range)
{
    bool Result = false;
    
    bool IsAfterStartTime = SystemTimeIsAfterTime(SysTime, Range.StartHour, Range.StartMinute);
    bool IsBeforeEndTime = SystemTimeIsBeforeTime(SysTime, Range.EndHour, Range.EndMinute);
    Result = IsAfterStartTime && IsBeforeEndTime;
    
    return Result;
}

internal bool
SystemTimeIsInTimeRangeList(system_time SysTime, time_range* Ranges, u32 RangesCount, time_range* RangeOut = 0)
{
    bool Result = false;
    for (u32 i = 0; i < RangesCount; i++)
    {
        time_range Range = Ranges[i];
        bool CurrTimeInRange = SystemTimeIsInTimeRange(SysTime, Range);
        if (CurrTimeInRange)
        {
            Result = true;
            if (RangeOut != 0) {
                *RangeOut = Range;
            }
            break;
        }
    }
    return Result;
}

#include "blumen_lumen_settings.h"

struct blumen_lumen_state
{
    bool Running;
    
    blumen_network_msg_queue IncomingMsgQueue;
    blumen_network_msg_queue OutgoingMsgQueue;
    
    temp_job_req JobReq;
    
    led_strip_list StemStrips[BL_FLOWER_COUNT];
    
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
    bl_motor_state_value LastSendState;
    
    phrase_hue StandardPatternHues;
    r32 AssemblyColorsTransitionTimeLeft[BL_FLOWER_COUNT];
    phrase_hue NextAssemblyColors[BL_FLOWER_COUNT];
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
    animation_handle OffAnimHandle;
    animation_handle AwakenHandle;
    
    phrase_hue_map PhraseHueMap;
    
    bool InPhraseReceptionMode;
    phrase_hue NextHotHue;
    system_time TimePhraseReceptionBegan;
    system_time TimeLastPhraseReceived;
    
    system_time TimeLastSetToVoiceMode;
    phrase_hue LastHuePhrase;
    
    r32 PatternSpeed;
    
    // Debug
    bl_debug_ui_mode DebugMode;
    
    motor_packet DEBUG_PendingMotorPacket;
    bool DEBUG_IgnoreWeatherDimmingLeds;
    
    bool ShouldUpdateLog;
    bool IgnoreTimeOfDay_LedDimming;
    bool IgnoreTimeOfDay_MotorState;
    
    phrase_hue PendingPhrase;
    
    bool DebugOverrideHue;
    phrase_hue DebugHue;
};

internal bool
Blumen_TempShouldDimLeds(blumen_lumen_state* BLState)
{
    bool Result = BLState->LastTemperatureReceived > MinHighTemperature;
    return Result;
}

#include "message_queue.cpp"

internal void
BlumenLumen_SetNextHue(blumen_lumen_state* BLState, u32 AssemblyIndex, phrase_hue Hue)
{
#if 1
    BLState->NextAssemblyColors[AssemblyIndex] = Hue;
    BLState->AssemblyColorsTransitionTimeLeft[AssemblyIndex] = PhraseHueFadeInDuration;
#else
    BLState->AssemblyColors[AssemblyIndex] = Hue;
#endif
}

internal void
BlumenLumen_AdvanceHueFade(blumen_lumen_state* BLState, context Context)
{
    for (u32 i = 0; i < BL_FLOWER_COUNT; i++)
    {
        r32 T = BLState->AssemblyColorsTransitionTimeLeft[i];
        if (T > 0)
        {
            T -= Context.DeltaTime;
            if (T <= 0)
            {
                BLState->AssemblyColors[i] = BLState->NextAssemblyColors[i];
            }
            BLState->AssemblyColorsTransitionTimeLeft[i] = T;
        }
    }
}

internal phrase_hue
BlumenLumen_GetCurrentHue(blumen_lumen_state* BLState, assembly Assembly)
{
    phrase_hue Result = {};
    
    switch (BLState->PatternMode)
    {
        case BlumenPattern_NoControl:
        case BlumenPattern_Standard:
        {
            Result = BLState->StandardPatternHues;
        }break;
        
        case BlumenPattern_VoiceCommand:
        {
            u32 i = Assembly.AssemblyIndex % 3;
            r32 T = BLState->AssemblyColorsTransitionTimeLeft[i];
            if (T > 0)
            {
                T = Clamp(T / PhraseHueFadeInDuration, 0, 1); 
                Result = LerpPhraseHue(T, BLState->NextAssemblyColors[i], BLState->AssemblyColors[i]);
            } else {
                Result = BLState->AssemblyColors[i];
            }
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
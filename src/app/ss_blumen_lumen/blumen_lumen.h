//
// File: blumen_lumen.h
// Author: Peter Slattery
// Creation Date: 2021-01-15
//
#ifndef BLUMEN_LUMEN_H

enum bl_python_packet_type
{
    PacketType_Invalid = 0,
    PacketType_PatternCommand = 1,
    PacketType_MotorState = 2,
    PacketType_Temperature = 3,
    PacketType_LumenariumStatus = 4,
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

#define BLUMEN_MESSAGE_QUEUE_COUNT 32
typedef struct blumen_network_msg_queue
{
    gs_data Buffers[BLUMEN_MESSAGE_QUEUE_COUNT];
    u32 WriteHead;
    u32 ReadHead;
} blumen_network_msg_queue;

// TODO(pjs): Refactor this -> blumen_network_job_state
struct mic_listen_job_data
{
    bool* Running;
    
    platform_socket_manager* SocketManager;
    blumen_network_msg_queue* IncomingMsgQueue;
    platform_socket_handle_ ListenSocket;
    
    blumen_network_msg_queue* OutgoingMsgQueue;
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

global time_range MotorOpenTimes[] = {
    { 17, 56, 17, 56 },
    { 17, 58, 17, 56 },
    { 18, 00, 18, 00 },
    
};
global u32 MotorOpenTimesCount = 3;

struct phrase_string_to_anim_file
{
    char* Phrase;
    u32 PatternIndex;
};

phrase_string_to_anim_file PhraseToAnimMap[] = {
    { "begonia", 0},
    { "hyacinth", 1 },
    { "tulip", 1 },
    { "calla lilly", 0 },
    { "sunflower", 1 },
    { "salvia", 2 },
    { "freesia", 2 },
};
u32 PhraseToAnimMapCount = sizeof(PhraseToAnimMap) / sizeof(PhraseToAnimMap[0]);

struct blumen_lumen_state
{
    bool Running;
    
    blumen_network_msg_queue IncomingMsgQueue;
    blumen_network_msg_queue OutgoingMsgQueue;
    
    temp_job_req JobReq;
    
    platform_thread_handle MicListenThread;
    mic_listen_job_data MicListenJobData;
    
    motor_packet LastKnownMotorState;
    
    r64 TimeElapsed;
    
    animation_handle AnimHandles[3];
    u32 CurrAnim;
    
    // NOTE(pjs): Based on temperature data from weatherman
    // dim the leds.
    r32 BrightnessPercent;
    system_time LastStatusUpdateTime;
    
    system_time LastSendTime;
};


















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
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
    { 00, 30, 00, 40 },
    { 00, 50, 01, 00 },
    { 01, 10, 01, 20 },
    { 01, 30, 01, 40 },
    { 01, 50, 02, 00 },
    { 02, 10, 02, 20 },
    { 02, 30, 02, 40 },
    { 02, 50, 03, 00 },
    { 03, 10, 03, 20 },
    { 03, 30, 03, 40 },
    { 03, 50, 04, 00 },
    { 04, 10, 04, 20 },
    { 04, 30, 04, 40 },
    { 04, 50, 05, 00 },
    { 05, 10, 05, 20 },
    { 05, 30, 05, 40 },
    { 05, 50, 06, 00 },
    { 06, 10, 06, 20 },
    { 06, 30, 06, 40 },
    { 06, 50, 07, 00 },
    { 07, 10, 07, 20 },
    { 07, 30, 07, 40 },
    { 07, 50,  8, 00 },
    {  8, 10,  8, 20 },
    {  8, 30,  8, 40 },
    {  8, 50,  9, 00 },
    {  9, 10,  9, 20 },
    {  9, 30,  9, 40 },
    {  9, 50, 10, 00 },
    { 10, 10, 10, 20 },
    { 10, 30, 10, 40 },
    { 10, 50, 11, 00 },
    { 11, 10, 11, 20 },
    { 11, 30, 11, 40 },
    { 11, 50, 12, 00 },
    { 12, 10, 12, 20 },
    { 12, 30, 12, 40 },
    { 12, 50, 13, 00 },
    { 13, 10, 13, 20 },
    { 13, 30, 13, 40 },
    { 13, 50, 14, 00 },
    { 14, 10, 14, 20 },
    { 14, 30, 14, 40 },
    { 14, 50, 15, 00 },
};
global u32 MotorOpenTimesCount = sizeof(MotorOpenTimes) / sizeof(MotorOpenTimes[0]);;

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
    
    v4 AssemblyColors[3];
    
    // The indices of this array are the index the clear core uses to 
    // represent a motor. 
    // The values of the array are the names Lumenarium uses to 
    // represent assemblies.
    // 
    u32 AssemblyNameToClearCoreMapCount;
    u64* AssemblyNameToClearCore_Names;
    
    // Debug
    motor_packet DEBUG_PendingMotorPacket;
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
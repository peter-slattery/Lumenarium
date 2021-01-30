//
// File: blumen_lumen.h
// Author: Peter Slattery
// Creation Date: 2021-01-15
//
#ifndef BLUMEN_LUMEN_H

typedef struct motor_packet
{
    u8 FlowerPositions[3];
} motor_packet;

#pragma pack(push, 1)
struct microphone_packet
{
    b8 ChangeAnimation;
    char AnimationFileName[32];
    b8 SetLayer;
    char LayerName[32];
    r32 LayerOpacity;
    b8 SetLayerParamColor;
    char LayerParamColor[7];
    r32 OverrideDuration;
};
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
    platform_socket_manager* SocketManager;
    packet_ringbuffer* MicPacketBuffer;
    platform_socket_handle_ ListenSocket;
    
    blumen_network_msg_queue* OutgoingMsgQueue;
};

struct blumen_lumen_state
{
    packet_ringbuffer MicPacketBuffer;
    blumen_network_msg_queue OutgoingMsgQueue;
    
    temp_job_req JobReq;
    
    
    platform_thread_handle MicListenThread;
    mic_listen_job_data MicListenJobData;
};


#define BLUMEN_LUMEN_H
#endif // BLUMEN_LUMEN_H
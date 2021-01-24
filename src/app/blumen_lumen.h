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

struct blumen_lumen_state
{
    packet_ringbuffer MicPacketBuffer;
    temp_job_req JobReq;
};


#define BLUMEN_LUMEN_H
#endif // BLUMEN_LUMEN_H
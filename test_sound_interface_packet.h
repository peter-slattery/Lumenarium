//
// File: test_sound_interface_packet.h
// Author: Peter Slattery
// Creation Date: 2021-01-14
//
#ifndef TEST_SOUND_INTERFACE_PACKET_H

enum selectable_patterns
{
};

typedef struct
{
    u8 PrimaryColor[3];
    u8 SecondaryColor[3];
    u32 PatternIndex; // Set via selectable_patterns
    u8 Speed; //
} sound_interface_packet;

// expects V to be in the range -1:1
u8 FloatToSpeed (float V)
{
    u8 Result = (u8)(((V + 1) / 2) * 255);
    return Result;
}


#define TEST_SOUND_INTERFACE_PACKET_H
#endif // TEST_SOUND_INTERFACE_PACKET_H
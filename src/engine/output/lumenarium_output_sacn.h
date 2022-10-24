/* date = March 30th 2022 9:55 am */

#ifndef LUMENARIUM_OUTPUT_SACN_H
#define LUMENARIUM_OUTPUT_SACN_H

#define SACN_CID_BYTES 16
typedef struct Sacn_Cid Sacn_Cid;
struct Sacn_Cid
{
  u8 bytes[SACN_CID_BYTES];
};

typedef struct Sacn Sacn;
struct Sacn
{
  Sacn_Cid cid;
  s32 sequence_iter;
  String source_name;
  Socket_Handle socket;
};

internal u8* output_network_sacn_init();
internal void output_network_sacn_update(u8* method_data);
internal void output_network_sacn_build(App_State* state, u32 assembly_id, Assembly_Pixel_Buffer* pixels, Assembly_Strip* strip, u8* method_data, Output_Data_Queue* queue);

#endif //LUMENARIUM_OUTPUT_SACN_H

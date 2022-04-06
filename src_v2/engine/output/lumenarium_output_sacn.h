/* date = March 30th 2022 9:55 am */

#ifndef LUMENARIUM_OUTPUT_SACN_H
#define LUMENARIUM_OUTPUT_SACN_H

#define SACN_CID_BYTES 16
struct Sacn_Cid
{
  u8 bytes[SACN_CID_BYTES];
};

struct Sacn
{
  Sacn_Cid cid;
  s32 sequence_iter;
  String source_name;
};

internal u8* output_network_sacn_init();
internal void output_network_sacn_build(App_State* state, u32 assembly_id, Assembly_Strip* strip, u8* method_data, Output_Data_Queue* queue);

#endif //LUMENARIUM_OUTPUT_SACN_H

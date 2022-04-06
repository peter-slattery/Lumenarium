/* date = March 30th 2022 9:23 am */

#ifndef LUMENARIUM_ENGINE_OUTPUT_H
#define LUMENARIUM_ENGINE_OUTPUT_H

typedef u8 Output_Data_Kind;
enum
{
  OutputData_Invalid = 0,
  OutputData_NetworkSACN,
  OutputData_ComUART,
  OutputData_Count,
};

struct Output_Data_Network
{
  // Platform_Socket_Handle socket;
  u32 v4_addr;
  u32 port;
};

struct Output_Data_Com
{
  String port;
};

struct Output_Data
{
  Output_Data_Kind kind;
  union
  {
    Output_Data_Network network;
    Output_Data_Com com;
  };
  Data data;
  
  Output_Data* next;
};

struct Output_Data_Queue
{
  Output_Data* first;
  Output_Data* last;
  u32 len;
  Allocator* a;
};

typedef void Build_Output_Data_Buffer(App_State* state, u32 assembly_id, Assembly_Strip* strip, u8* method_data, Output_Data_Queue* queue);

struct Output_Methods
{
  Build_Output_Data_Buffer* procs[OutputData_Count];
  u8* method_data[OutputData_Count];
};

struct Output
{
  Output_Methods methods;
};

internal void register_output_method(Output* output, Output_Data_Kind kind, Build_Output_Data_Buffer* proc, u8* method_data);

internal Output_Data* output_data_queue_push(Output_Data_Queue* q, u32 size, Output_Data_Kind kind);

internal void output_data_set_network_addr(Output_Data* d, u32 send_addr, u32 port);

#endif //LUMENARIUM_ENGINE_OUTPUT_H

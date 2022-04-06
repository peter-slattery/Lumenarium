#define SACN_DEFAULT_PORT 5568


// a description of the address space being used
#define SACN_PREAMBLE_SIZE_ADDR 0
#define SACN_POSTAMBLE_SIZE_ADDR 2
#define SACN_ACN_IDENTIFIER_ADDR 4
#define SACN_ROOT_FLAGS_AND_LEN_ADDR 16
#define SACN_ROOT_VECTOR_ADDR 18
#define SACN_CID_ADDR 22
#define SACN_FRAMING_FLAGS_AND_LEN_ADDR 38
#define SACN_FRAMING_VECTOR_ADDR 40
#define SACN_SOURCE_NAME_ADDR 44
#define SACN_PRIORITY_ADDR 108
#define SACN_RESERVED_ADDR 109
#define SACN_SEQ_NUM_ADDR 111
#define SACN_OPTIONS_ADDR 112
#define SACN_UNIVERSE_ADDR 113
#define SACN_DMP_FLAGS_AND_LEN_ADDR 115
#define SACN_DMP_VECTOR_ADDR 117
#define SACN_DMP_ADDRESS_AND_DATA_ADDR 118
#define SACN_FIRST_PROPERTY_ADDRESS_ADDR 119
#define SACN_ADDRESS_INC_ADDR 121
#define SACN_PROP_COUNT_ADDR 123
#define SACN_START_CODE_ADDR 125
#define SACN_PROP_VALUES_ADDR (SACN_START_CODE_ADDR + 1)

// Common Sizes
#define SACN_BUFFER_HEADER_SIZE 126
#define SACN_BUFFER_BODY_SIZE   512
#define SACN_BUFFER_SIZE       (SACN_BUFFER_HEADER_SIZE + SACN_BUFFER_BODY_SIZE)

#define SACN_SOURCE_NAME_SIZE 64
#define SACN_ACN_IDENTIFIER_SIZE 12
#define SACN_RLP_PREAMBLE_SIZE  16
#define SACN_RLP_POSTAMBLE_SIZE 0

// Data Definitions
#define SACN_ACN_IDENTIFIER     lit_str("ASC-E1.17\0\0\0")
#define SACN_ROOT_VECTOR 4
#define SACN_FRAMING_VECTOR 2
#define SACN_DMP_VECTOR 2
#define SACN_ADDRESS_AND_DATA_FORMAT 0xa1
#define SACN_ADDR_INC 1
#define SACN_DMP_FIRST_PROPERTY_ADDRESS_FORCE 0
#define SACN_RESERVED_VALUE 0

#define SACN_VHD_L_FLAG 0x80
#define SACN_VHD_V_FLAG 0x40
#define SACN_VHD_H_FLAG 0x20
#define SACN_VHD_D_FLAG 0x10

#define SACN_VHD_MAXFLAGBYTES 7 //The maximum amount of bytes used to pack the flags, len, and vector
#define SACN_VHD_MAXLEN 0x0fffff  //The maximum packet length is 20 bytes long
#define SACN_VHD_MAXMINLENGTH 4095  //The highest length that will fit in the "smallest" length pack


internal void
sacn_vhd_pack_flags(Data_Writer* w, b8 inherit_vec, b8 inherit_head, b8 inherit_data)
{
  u8 last_byte = dw_get_u8(w) & 0x8F;
  w->at -= 1;
  
  if (!inherit_vec)  { last_byte |= SACN_VHD_V_FLAG; }
  if (!inherit_head) { last_byte |= SACN_VHD_H_FLAG; }
  if (!inherit_data) { last_byte |= SACN_VHD_D_FLAG; }
  
  dw_put_u8(w, last_byte);
}

internal void
sacn_vhd_pack_len(Data_Writer* w, u32 len, b8 include_len)
{
  u32 len_adjusted = len;
  if (include_len)
  {
    if (len + 1 > SACN_VHD_MAXMINLENGTH)
    {
      len_adjusted += 2;
    }
    else
    {
      len_adjusted += 1;
    }
  }
  
  // Mask out the length bits to keep flags intact
  u8 last_byte = dw_get_u8(w) & 0x70;
  w->at -= 1;
  
  if (len_adjusted > SACN_VHD_MAXMINLENGTH) last_byte |= SACN_VHD_L_FLAG;
  
  u8* pack_buffer = (u8*)&len_adjusted;
  if (len_adjusted <= SACN_VHD_MAXMINLENGTH)
  {
    last_byte |= (pack_buffer[1] & 0x0f);
    dw_put_u8(w, last_byte);
    dw_put_u8(w, pack_buffer[0]);
  }
  else
  {
    last_byte |= (pack_buffer[2] & 0x0f);
    dw_put_u8(w, pack_buffer[1]);
    dw_put_u8(w, pack_buffer[0]);
  }
}

internal void
sacn_fill_buffer_header(Output_Data* d, u16 universe, Sacn* sacn)
{
  assert(d && d->data.size > 0);
  
  // TODO(PS): these should be passed in?
  u16 slot_count = 0; 
  u8 start_code = 0;
  u8 priority = 0;
  u16 reserved = 0;
  u8 options = 0;
  
  Data_Writer w = {};
  w.data = d->data;
  
  dw_put_u16_b(&w, SACN_RLP_PREAMBLE_SIZE);
  dw_put_u16_b(&w, SACN_RLP_POSTAMBLE_SIZE);
  dw_put_str(&w, SACN_ACN_IDENTIFIER);
  
  sacn_vhd_pack_flags(&w, false, false, false);
  sacn_vhd_pack_len(&w, SACN_BUFFER_HEADER_SIZE - SACN_RLP_PREAMBLE_SIZE + slot_count, false);
  
  dw_put_u32_b(&w, SACN_ROOT_VECTOR);
  
  for (u32 i = 0; i < SACN_CID_BYTES; i++) dw_put_u8(&w, sacn->cid.bytes[i]);
  
  sacn_vhd_pack_flags(&w, false, false, false);
  sacn_vhd_pack_len(&w, SACN_BUFFER_HEADER_SIZE - SACN_FRAMING_FLAGS_AND_LEN_ADDR + slot_count, false);
  
  dw_put_u32_b(&w, SACN_FRAMING_VECTOR);
  dw_put_str(&w, sacn->source_name);
  
  dw_put_u8(&w, priority);
  dw_put_u16_b(&w, reserved);
  dw_put_u8(&w, 0); // Sequence Number starts at 0, gets updated
  dw_put_u8(&w, options);
  dw_put_u16_b(&w, universe);
  
  sacn_vhd_pack_flags(&w, false, false, false);
  sacn_vhd_pack_len(&w, SACN_BUFFER_HEADER_SIZE - SACN_DMP_FLAGS_AND_LEN_ADDR + slot_count, false);
  
  dw_put_u8(&w, SACN_DMP_VECTOR);
  dw_put_u8(&w, SACN_ADDRESS_AND_DATA_FORMAT);
  dw_put_u8(&w, 0); // dmp first priority addr
  dw_put_u8(&w, SACN_ADDR_INC); // dmp address increment
  dw_put_u16_b(&w, slot_count + 1);
  dw_put_u8(&w, start_code);
  
  assert(w.at == SACN_BUFFER_HEADER_SIZE);
}

internal void
sacn_fill_buffer_body(Output_Data* d, u32* leds_placed)
{
  
}

internal Sacn_Cid
sacn_string_to_cid(String str)
{
  return {};
}

internal u32
sacn_universe_to_send_addr(u32 universe)
{
  return 0;
}

internal u8* 
output_network_sacn_init()
{
  Sacn* result = allocator_alloc_struct(permanent, Sacn);
  // TODO(PS): get platform send socket
  
  String cid_str = lit_str("{67F9D986-544E-4abb-8986-D5F79382586C}");
  result->cid = sacn_string_to_cid(cid_str);
  
  return (u8*)result;
}

internal void
output_network_sacn_build(App_State* state, u32 assembly_id, Assembly_Strip* strip, u8* method_data, Output_Data_Queue* queue)
{
  Sacn* sacn = (Sacn*)method_data;
  
  u16 universe = (u16)strip->sacn_universe;
  u32 send_port = SACN_DEFAULT_PORT;
  for (u32 leds_placed = 0; leds_placed < strip->pixels_len;)
  {
    u32 v4_send_addr = sacn_universe_to_send_addr(universe);
    
    Output_Data* d = output_data_queue_push(queue, SACN_BUFFER_SIZE, OutputData_NetworkSACN);
    output_data_set_network_addr(d, v4_send_addr, send_port);
    
    sacn_fill_buffer_header(d, universe, sacn);
    sacn_fill_buffer_body(d, &leds_placed);
    universe += 1;
  }
}

#define SACN_DEFAULT_PORT 5568

#define SACN_STARTCODE_DMX 0

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
  
  if (!inherit_vec)  { last_byte |= SACN_VHD_V_FLAG; }
  if (!inherit_head) { last_byte |= SACN_VHD_H_FLAG; }
  if (!inherit_data) { last_byte |= SACN_VHD_D_FLAG; }
  
  w->data.base[w->at] = last_byte;
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

#define CopyMemoryTo(from, to, size) CopyMemory_((u8*)(from), (u8*)(to), (size))
internal void
CopyMemory_(u8* From, u8* To, u64 Size)
{
  for (u64 i = 0; i < Size; i++)
  {
    To[i] = From[i];
  }
}

// Packs a u8 to a known big endian buffer
u8*
PackB1(u8* ptr, u8 val)
{
	*ptr = val;
  return ptr + sizeof(val);
}

//Unpacks a u8 from a known big endian buffer
u8
UpackB1(const u8* ptr)
{
	return *ptr;
}

//Packs a u8 to a known little endian buffer
u8*
PackL1(u8* ptr, u8 val)
{
	*ptr = val;
  return ptr + sizeof(val);
}

//Unpacks a u8 from a known little endian buffer
u8
UpackL1(const u8* ptr)
{
	return *ptr;
}

u8*
PackB2(u8* ptr, u16 val)
{
  ptr[1] = (u8)(val & 0xff);
	ptr[0] = (u8)((val & 0xff00) >> 8);
  return ptr + sizeof(val);
}

//Unpacks a u16 from a known big endian buffer
u16
UpackB2(const u8* ptr)
{
	return (u16)(ptr[1] | ptr[0] << 8);
}

//Packs a u32 to a known big endian buffer
u8*
PackB4(u8* ptr, u32 val)
{
	ptr[3] = (u8) (val & 0xff);
	ptr[2] = (u8)((val & 0xff00) >> 8);
	ptr[1] = (u8)((val & 0xff0000) >> 16);
	ptr[0] = (u8)((val & 0xff000000) >> 24);
  return ptr + sizeof(val);
}

internal void
VHD_PackFlags_(u8* Buffer, b32 InheritVec, b32 InheritHead, b32 InheritData)
{
  u8* Cursor = Buffer;
  u8 NewByte = UpackB1(Cursor) & 0x8f;
  
  if (!InheritVec) { NewByte |= SACN_VHD_V_FLAG; }
  if (!InheritHead) { NewByte |= SACN_VHD_H_FLAG; }
  if (!InheritData) { NewByte |= SACN_VHD_D_FLAG; }
  
  PackB1(Cursor, NewByte);
}

internal u8*
VHD_PackLength_(u8* Buffer, u32 Length, b32 IncludeLength)
{
  u8* Cursor = Buffer;
  u32 AdjustedLength = Length;
  if (IncludeLength)
  {
    if (Length + 1 > SACN_VHD_MAXMINLENGTH)
    {
      AdjustedLength += 2;
    }
    else
    {
      AdjustedLength += 1;
    }
  }
  
  // Mask out the length bits to keep flags intact
  u8 NewByte = UpackB1(Cursor) & 0x70;
  if (AdjustedLength > SACN_VHD_MAXMINLENGTH)
  {
    NewByte |= SACN_VHD_L_FLAG;
  }
  
  u8 PackBuffer[4];
  PackB4(PackBuffer, AdjustedLength);
  if (AdjustedLength <= SACN_VHD_MAXMINLENGTH)
  {
    NewByte |= (PackBuffer[2] & 0x0f);
    Cursor = PackB1(Cursor, NewByte);
    Cursor = PackB1(Cursor, PackBuffer[3]);
  }
  else
  {
    NewByte |= (PackBuffer[1] & 0x0f);
    Cursor = PackB1(Cursor, PackBuffer[2]);
    Cursor = PackB1(Cursor, PackBuffer[3]);
  }
  
  return Cursor;
}

internal void
InitStreamHeader (u8* Buffer, s32 BufferSize,
  u16 SlotCount,
  u8 StartCode,
  u16 Universe,
  u8 Priority,
  u16 Reserved,
  u8 Options,
  const char* SourceName,
  Sacn_Cid CID
)
{
  // TODO(pjs): Replace packing with gs_memory_cursor
  
  u8* Cursor = Buffer;
  
  // Preamble Size
  Cursor = PackB2(Cursor, SACN_RLP_PREAMBLE_SIZE);
  Cursor = PackB2(Cursor, SACN_RLP_POSTAMBLE_SIZE);
  
  CopyMemoryTo(SACN_ACN_IDENTIFIER.str, Cursor, SACN_ACN_IDENTIFIER_SIZE);
  Cursor += SACN_ACN_IDENTIFIER_SIZE;
  
  // TODO(Peter): If you never use this anywhere else, go back and remove the parameters
  VHD_PackFlags_(Cursor, false, false, false);
  Cursor = VHD_PackLength_(Cursor,
    SACN_BUFFER_HEADER_SIZE - SACN_RLP_PREAMBLE_SIZE + SlotCount,
    false);
  
  // root vector
  Cursor = PackB4(Cursor, SACN_ROOT_VECTOR); // 22
  
  // CID Pack
  for (s32 i = 0; i < SACN_CID_BYTES; i++)
  {
    *Cursor++ = CID.bytes[i];
  }// 38
  
  VHD_PackFlags_(Cursor, false, false, false);
  Cursor = VHD_PackLength_(Cursor,
    SACN_BUFFER_HEADER_SIZE - SACN_FRAMING_FLAGS_AND_LEN_ADDR + SlotCount,
    false);
  // 40
  // framing vector
  Cursor = PackB4(Cursor, SACN_FRAMING_VECTOR);
  
  // framing source name
  // :Check
  
  CopyMemoryTo(SourceName, (char*)Cursor, c_str_len((char*)SourceName));
  Cursor[SACN_SOURCE_NAME_SIZE - 1] = '\0';
  Cursor += SACN_SOURCE_NAME_SIZE; // 108
  
  // priority
  Cursor = PackB1(Cursor, Priority);
  
  // reserved
  Cursor = PackB2(Cursor, Reserved); // 111
  
  // Sequence # is always set to 0/NONE at the beginning, but it is incremented when sending data
  Cursor = PackB1(Cursor, 0);
  
  // Options
  Cursor = PackB1(Cursor, Options);
  
  // Universe
  Cursor = PackB2(Cursor, Universe); // 115
  
  VHD_PackFlags_(Cursor, false, false, false);
  Cursor = VHD_PackLength_(Cursor,
    SACN_BUFFER_HEADER_SIZE - SACN_DMP_FLAGS_AND_LEN_ADDR + SlotCount,
    false); // 117
  
  // DMP Vector
  Cursor = PackB1(Cursor, SACN_DMP_VECTOR);
  
  // DMP Address and data type
  Cursor = PackB1(Cursor, SACN_ADDRESS_AND_DATA_FORMAT);
  
  // DMP first property address
  Cursor = PackB1(Cursor, 0);
  
  // DMP Address Increment
  Cursor = PackB1(Cursor, SACN_ADDR_INC);
  
  // Property Value Count -- Includes one byte for start code
  Cursor = PackB2(Cursor, SlotCount + 1);
  
  Cursor = PackB1(Cursor, StartCode);
  
  assert(Cursor - Buffer == SACN_BUFFER_HEADER_SIZE);
  
}

internal void
sacn_fill_buffer_header(Output_Data* d, u16 universe, Sacn* sacn)
{
  assert(d && d->data.size > 0);
  
  // TODO(PS): these should be passed in?
  u16 slot_count = SACN_BUFFER_BODY_SIZE; 
  u8 start_code = SACN_STARTCODE_DMX;
  // universe
  u8 priority = 0;
  u16 reserved = 0;
  u8 options = 0;
  
  Data_Writer w = {};
  w.data = d->data;
  
  dw_put_u16_b(&w, SACN_RLP_PREAMBLE_SIZE);
  dw_put_u16_b(&w, SACN_RLP_POSTAMBLE_SIZE);
  dw_put_str_min_len(&w, SACN_ACN_IDENTIFIER, SACN_ACN_IDENTIFIER_SIZE);
  
  sacn_vhd_pack_flags(&w, false, false, false);
  sacn_vhd_pack_len(&w, SACN_BUFFER_HEADER_SIZE - SACN_RLP_PREAMBLE_SIZE + slot_count, false);
  
  dw_put_u32_b(&w, SACN_ROOT_VECTOR);
  
  for (u32 i = 0; i < SACN_CID_BYTES; i++) dw_put_u8(&w, sacn->cid.bytes[i]);
  
  sacn_vhd_pack_flags(&w, false, false, false);
  sacn_vhd_pack_len(&w, SACN_BUFFER_HEADER_SIZE - SACN_FRAMING_FLAGS_AND_LEN_ADDR + slot_count, false);
  
  dw_put_u32_b(&w, SACN_FRAMING_VECTOR);
  
  dw_put_str_min_len_nullterm(&w, sacn->source_name, SACN_SOURCE_NAME_SIZE);
  
  dw_put_u8(&w, priority);
  dw_put_u16_b(&w, reserved); // synchronization 
  dw_put_u8(&w, sacn->sequence_iter);
  dw_put_u8(&w, options);
  dw_put_u16_b(&w, universe);
  
  sacn_vhd_pack_flags(&w, false, false, false);
  sacn_vhd_pack_len(&w, SACN_BUFFER_HEADER_SIZE - SACN_DMP_FLAGS_AND_LEN_ADDR + slot_count, false);
  
  dw_put_u8(&w, SACN_DMP_VECTOR);
  dw_put_u8(&w, SACN_ADDRESS_AND_DATA_FORMAT);
  dw_put_u16_b(&w, 0); // dmp first priority addr
  dw_put_u16_b(&w, SACN_ADDR_INC); // dmp address increment
  dw_put_u16_b(&w, slot_count + 1);
  dw_put_u8(&w, start_code);
  
  assert(w.at == SACN_BUFFER_HEADER_SIZE);
}

internal void
sacn_fill_buffer_body(Output_Data* d, Assembly_Pixel_Buffer* pixels, Assembly_Strip* strip, u32* leds_placed)
{
  u32 first = *leds_placed;
  u32 to_add = min(strip->pixels_len - first, SACN_BUFFER_BODY_SIZE / 3);
  u32 one_past_last = first + to_add;
  for (u32 i = *leds_placed; i < one_past_last; i++)
  {
    u32 led_index = strip->pixels[i];
    Assembly_Pixel color = pixels->pixels[led_index];
    d->data.base[SACN_BUFFER_HEADER_SIZE + (i * 3) + 0] = color.r;
    d->data.base[SACN_BUFFER_HEADER_SIZE + (i * 3) + 1] = color.g;
    d->data.base[SACN_BUFFER_HEADER_SIZE + (i * 3) + 2] = color.b; 
  }
  *leds_placed += to_add;
}

internal Sacn_Cid
sacn_string_to_cid(String str)
{
  return (Sacn_Cid){};
}

//Unpacks a u32 from a known big endian buffer
inline u32
UpackB4(const u8* ptr)
{
	return (u32)(ptr[3] | (ptr[2] << 8) | (ptr[1] << 16) | (ptr[0] << 24));
}

internal u32
sacn_universe_to_send_addr(u32 universe)
{
  u8 multicast_address_buffer[4] = {};
  multicast_address_buffer[0] = 239;
  multicast_address_buffer[1] = 255;
  multicast_address_buffer[2] = (u8)((universe & 0xff00) >> 8); // high bit
  multicast_address_buffer[3] = (u8)((universe & 0x00ff)); // low bit
  
  u32 v4_address = (u32)((multicast_address_buffer[3]      ) |
    (multicast_address_buffer[2] <<  8) |
    (multicast_address_buffer[1] << 16) |
    (multicast_address_buffer[0] << 24));
  return v4_address;
}

internal u8* 
output_network_sacn_init()
{
  Sacn* result = allocator_alloc_struct(permanent, Sacn);
  zero_struct(*result);
  
  result->source_name = string_f(permanent, "lumenarium::incenter");
  
  String cid_str = lit_str("{67F9D986-544E-4abb-8986-D5F79382586C}");
  result->cid = sacn_string_to_cid(cid_str);
  
  s32 ttl = 20;
  result->socket = os_socket_create(AF_INET, SOCK_DGRAM, 0);
  os_socket_set_opt(
      result->socket, 
    IPPROTO_IP, 
    IP_MULTICAST_TTL, 
    (u8*)&ttl, sizeof(ttl)
  );
  
  return (u8*)result;
}

internal void
output_network_sacn_update(u8* method_data)
{
  Sacn* sacn = (Sacn*)method_data;
  sacn->sequence_iter += 1;
}

internal void
output_network_sacn_build(App_State* state, u32 assembly_id, Assembly_Pixel_Buffer* pixels, Assembly_Strip* strip, u8* method_data, Output_Data_Queue* queue)
{
  Sacn* sacn = (Sacn*)method_data;
  
  u16 universe = (u16)strip->sacn_universe;
  u32 send_port = SACN_DEFAULT_PORT;
  for (u32 leds_placed = 0; leds_placed < strip->pixels_len;)
  {
    u32 v4_send_addr = sacn_universe_to_send_addr(universe);
    
    Output_Data* d = output_data_queue_push(queue, SACN_BUFFER_SIZE, OutputData_NetworkSACN);
    output_data_set_network_addr(d, v4_send_addr, send_port);
    
    //InitStreamHeader(d->data.base, d->data.size, SACN_BUFFER_BODY_SIZE, SACN_STARTCODE_DMX, universe, 0, 0, 0, "lumenarium::sacn", sacn->cid);
    sacn_fill_buffer_header(d, universe, sacn);
    sacn_fill_buffer_body(d, pixels, strip, &leds_placed);
    universe += 1;
  }
}

/* date = April 1st 2022 7:29 pm */

#ifndef LUMENARIUM_HASH_CPP
#define LUMENARIUM_HASH_CPP

//
// DJB2
// Source: http://www.cse.yorku.ca/~oz/hash.html

internal u32
hash_djb2_to_u32(char* str, u64 len)
{
  u32 result = 5381;
  for (u64 i = 0; i < len; i++)
  {
    result = ((result << 5) + result) + (u8)str[i];
  }
  return result;
}

internal u32
hash_djb2_to_u32(char* str)
{
  u64 len = c_str_len(str);
  return hash_djb2_to_u32(str, len);
}

internal u32
hash_djb2_to_u32(String str)
{
  return hash_djb2_to_u32((char*)str.str, str.len);
}

internal u64
hash_djb2_to_u64(char* str, u64 len)
{
  u64 result = 5381;
  for (u64 i = 0; i < len; i++)
  {
    result = ((result << 5) + result) + (u8)str[i];
  }
  return result;
}

internal u64
hash_djb2_to_u64(char* str)
{
  u64 len = c_str_len(str);
  return hash_djb2_to_u64(str, len);
}

internal u64
hash_djb2_to_u64(String str)
{
  return hash_djb2_to_u64((char*)str.str, str.len);
}

#endif //LUMENARIUM_HASH_CPP

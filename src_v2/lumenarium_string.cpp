
internal u64
c_str_len(char* s)
{
  u64 result = 0;
  for (; s[result] != 0; result++) {}
  return result;
}

#define str_varg(s) (int)(s).len, (char*)(s).str
#define str_expand(s) (char*)(s).str, (u64)(s).len
#define lit_str(s) String{ (u8*)(s), (u64)sizeof(s)-1, (u64)sizeof(s)-1 } 

internal String
allocator_alloc_string(Allocator* a, u64 cap) 
{
  String result = {};
  result.str = allocator_alloc_array(a, u8, cap);
  result.cap = cap;
  return result;
}

/////////////////////////////////////
// Char Operations

bool
char_is_space(u8 c)
{
  return c == ' ' || c == '\r' || c == '\t' || c == '\f' || c == '\v' || c == '\n';
}

u8
char_to_upper(u8 c)
{
  return (c >= 'a' && c <= 'z') ? ('A' + (c - 'a')) : c;
}

u8
char_to_lower(u8 c)
{
  return (c >= 'A' && c <= 'Z') ? ('a' + (c - 'A')) : c;
}

u8
char_to_forward_slash(u8 c)
{
  return (c == '\\' ? '/' : c);
}


/////////////////////////////////////
// String Operations
//
// Note that these don't actually modify any memory
// just return structures that let you view it differently

internal String
string_create(u8* str, u64 len, u64 cap)
{
  String result = {};
  result.str = str;
  result.len = len;
  result.cap = cap;
  return result;
}

internal String
string_substring(String s, u64 min, u64 max)
{
  if (max > s.len) max = s.len;
  if (min > s.len) min = s.len;
  if (min > max) {
    u64 t = min;
    min = max;
    max = t;
  }
  String result = {};
  result.str = s.str + min;
  result.len = max - min;
  result.cap = result.len;
  return result;
}

internal String
string_skip(String s, u64 min)
{
  return string_substring(s, min, s.len);
}

internal String
string_chop(String s, u64 nmax)
{
  return string_substring(s, 0, s.len - nmax);
}

internal String
string_get_prefix(String s, u64 max)
{
  return string_substring(s, 0, max);
}

internal String
string_get_suffix(String s, u64 nmax)
{
  return string_substring(s, s.len - nmax, s.len);
}

typedef u32 String_Match_Flags;
enum 
{
  StringMatch_FindLast = 1,
  StringMatch_CaseInsensitive = 2,
  StringMatch_SlashInsensitive = 4,
};

internal bool
string_match(String a, String b, String_Match_Flags flags)
{
  bool result = false;
  if (a.len == b.len)
  {
    result = true;
    for (u64 i = 0; i < a.len; i++)
    {
      bool match = a.str[i] == b.str[i];
      if(flags & StringMatch_CaseInsensitive)
      {
        match |= (char_to_lower(a.str[i]) == char_to_lower(b.str[i]));
      }
      if(flags & StringMatch_SlashInsensitive)
      {
        match |= (char_to_forward_slash(a.str[i]) == char_to_forward_slash(b.str[i]));
      }
      if(match == 0)
      {
        result = 0;
        break;
      }
    }
  }
  return result;
}

internal u64
string_find_substring(String s, String substr, u64 start_pos, String_Match_Flags flags)
{
  bool found = false;
  u64  found_i = s.len;
  for (u64 i = start_pos; i < s.len; i++)
  {
    if (i + substr.len <= s.len)
    {
      String at = string_substring(s, i, i + substr.len);
      if (string_match(at, substr, flags))
      {
        found = true;
        found_i = i;
        if (!(flags & StringMatch_FindLast)) break;
      }
    }
  }
  return found_i;
}

/////////////////////////////////////
// Path Operations

// good for removing extensions
internal String
string_chop_last_period(String s)
{
  u64 period_pos = string_find_substring(s, lit_str("."), 0, StringMatch_FindLast);
  if(period_pos < s.len)
  {
    s.len = period_pos;
    s.cap = s.len;
  }
  return s;
}

// get the filename
internal String
string_skip_last_slash(String s)
{
  u64 slash_pos = string_find_substring(s, lit_str("/"), 0, StringMatch_FindLast | StringMatch_SlashInsensitive);
  if(slash_pos < s.len)
  {
    s.str += slash_pos + 1;
    s.len -= slash_pos + 1;
    s.cap = s.len;
  }
  return s;
}

// get the extension
internal String
string_skip_last_period(String s)
{
  u64 period_pos = string_find_substring(s, lit_str("."), 0, StringMatch_FindLast);
  if(period_pos < s.len)
  {
    s.str += period_pos + 1;
    s.len -= period_pos + 1;
    s.cap = s.len;
  }
  return s;
}

// good for getting the path to a file
internal String
string_chop_last_slash(String s)
{
  u64 slash_pos = string_find_substring(s, lit_str("/"), 0, StringMatch_FindLast | StringMatch_SlashInsensitive);
  if(slash_pos < s.len)
  {
    s.len = slash_pos;
    s.cap = s.len;
  }
  return s;
}


/////////////////////////////////////
// String Modifications

internal u64
string_copy_to(String* dest, String src)
{
  u64 len_to_copy = dest->cap < src.len ? dest->cap : src.len;
  memory_copy(src.str, dest->str, len_to_copy);
  u64 null_term_index = len_to_copy;
  if (null_term_index >= dest->cap) null_term_index -= 1;
  dest->str[null_term_index] = 0;
  dest->len = null_term_index;
  return null_term_index;
}

internal String
string_copy(String s, Allocator* a)
{
  u64 size = s.cap;
  if (s.str[s.cap] != 0) size += 1;
  String result = allocator_alloc_string(a, size);
  string_copy_to(&result, s);
  return result;
}

internal String
string_fv(Allocator* a, char* fmt, va_list args)
{
  va_list args1;
  va_copy(args1, args);
  s32 needed = stbsp_vsnprintf(0, 0, fmt, args);
  String result = allocator_alloc_string(a, needed + 1);
  result.len = stbsp_vsnprintf((char*)result.str, (int)result.cap, fmt, args1);
  result.str[result.len] = 0;
  va_end(args1);
  return result;
}

internal String
string_f(Allocator* a, char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  String result = string_fv(a, fmt, args);
  va_end(args);
  return result;
}
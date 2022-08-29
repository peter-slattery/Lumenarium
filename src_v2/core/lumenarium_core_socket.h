#if !defined(LUMENARIUM_CORE_SOCKET_H)
#define LUMENARIUM_CORE_SOCKET_H

typedef struct Socket_Handle Socket_Handle;
struct Socket_Handle { u64 value; };

typedef s32 Socket_Error;
enum {
  SocketError_Invalid,
  SocketError_NOERROR,
  SocketError_EAGAIN,
  SocketError_EBADF,
  SocketError_ECONNREFUSED,
  SocketError_EFAULT,
  SocketError_EINTR,
  SocketError_EINVAL,
  SocketError_ENOMEM,
  SocketError_ENOTCONN,
  SocketError_ENOTSOCK,
  SocketError_Count,
};

global char* socket_error_strings[] = {
  "Invalid",
  "SocketError_NOERROR",
  "SocketError_EAGAIN",
  "SocketError_EBADF",
  "SocketError_ECONNREFUSED",
  "SocketError_EFAULT",
  "SocketError_EINTR",
  "SocketError_EINVAL",
  "SocketError_ENOMEM",
  "SocketError_ENOTCONN",
  "SocketError_ENOTSOCK",
};

u16 endian_swap_u16(u16 v);
u32 endian_swap_u32(u32 v);
u64 endian_swap_u64(u64 v);

#define hton_u16(v) endian_swap_u16(v)
#define hton_u32(v) endian_swap_u32(v)
#define hton_u64(v) endian_swap_u64(v)
#define ntoh_s16(v) endian_swap_u16(v)
#define ntoh_s32(v) endian_swap_u32(v)
#define ntoh_s64(v) endian_swap_u64(v)


//////////////////////////////////////////
// Implementation

u16
endian_swap_u16(u16 v)
{
  u8* p = (u8*)&v;
  u16 result = (u16)(
    (p[0] << 8) |
    (p[1] << 0)
  );
  return result;
}

u32
endian_swap_u32(u32 v)
{
  u8* p = (u8*)&v;
  u32 result = (u32)(
    (p[0] << 24) |
    (p[1] << 16) |
    (p[2] <<  8) |
    (p[3] <<  0)
  );
  return result;
}

u64
endian_swap_u64(u64 v)
{
  u8* p = (u8*)&v;
  u64 result = (u64)(
    ((u64)p[0] << 56) |
    ((u64)p[1] << 48) |
    ((u64)p[2] << 40) |
    ((u64)p[3] << 32) |
    ((u64)p[4] << 24) |
    ((u64)p[5] << 16) |
    ((u64)p[6] <<  8) |
    ((u64)p[7] <<  0)
  );
  return result;
}

#if defined(DEBUG)

void 
core_socket_tests()
{
  // u16 endian swap
  u16 a_0 = 0xABCD;
  u16 a_1 = endian_swap_u16(a_0);
  u16 a_2 = endian_swap_u16(a_1);
  assert(a_1 == 0xCDAB);
  assert(a_2 == a_0);

  // u32 endian swap
  u32 b_0 = 0x89ABCDEF;
  u32 b_1 = endian_swap_u32(b_0);
  u32 b_2 = endian_swap_u32(b_1);
  assert(b_1 == 0xEFCDAB89);
  assert(b_2 == b_0);

  // u64 endian swap
  u64 c_0 = 0x7654321089ABCDEF;
  u64 c_1 = endian_swap_u64(c_0);
  u64 c_2 = endian_swap_u64(c_1);
  assert(c_1 == 0xEFCDAB8910325476);
  assert(c_2 == c_0);
}

#else
#  define core_socket_tests() 
#endif

#endif // LUMENARIUM_CORE_SOCKET_H
#if !defined(OS_SOCKET_TYPE)
#  error "You must define an OS_SOCKET_TYPE"
#endif

#if !defined(OS_SOCKET_INVALID_HANDLE)
#  error "You must define an OS_SOCKET_INVALID_HANDLE"
#endif

#define               open_sockets_cap   3
global u32            open_sockets_len = 1;
global OS_SOCKET_TYPE open_sockets[open_sockets_cap];

void           open_sockets_init();
s32            open_sockets_next_free();
bool           open_sockets_has_room();
OS_SOCKET_TYPE open_sockets_get(Socket_Handle handle);
Socket_Handle  open_sockets_put(OS_SOCKET_TYPE socket);
void           open_sockets_rem(Socket_Handle handle);

////////////////////////////////////////////////
// IMPLEMENTATION

void           
open_sockets_init()
{
  for (u32 i = 0; i < open_sockets_cap; i++) 
  {
    open_sockets[i] = OS_SOCKET_INVALID_HANDLE;
  }
}

s32        
open_sockets_next_free()
{
  if (open_sockets_len < open_sockets_cap) return open_sockets_len++;
  for (u32 i = 1; i < open_sockets_len; i++)
  {
    if (open_sockets[i] == OS_SOCKET_INVALID_HANDLE) return i;
  }
  return 0;
}

OS_SOCKET_TYPE 
open_sockets_get(Socket_Handle handle)
{
  assert(handle.value < open_sockets_len);
  return open_sockets[handle.value];
}

Socket_Handle  
open_sockets_put(OS_SOCKET_TYPE socket)
{
  Socket_Handle result = { .value = open_sockets_next_free() };
  assert(result.value != 0);
  open_sockets[result.value] = socket;
  return result;
}

void           
open_sockets_rem(Socket_Handle handle)
{
  assert(handle.value < open_sockets_len);
  open_sockets[handle.value] = OS_SOCKET_INVALID_HANDLE;
  if (handle.value + 1 == open_sockets_len) {
    open_sockets_len -= 1;
  }
}

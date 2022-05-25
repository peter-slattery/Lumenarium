#ifndef LUMENARIUM_LINUX_NETWORK_H
#define LUMENARIUM_LINUX_NETWORK_H 1

Socket_Handle 
os_socket_create(s32 domain, s32 type, s32 protocol)
{
  invalid_code_path;
  return (Socket_Handle){0};
}

bool          
os_socket_bind()
{
  invalid_code_path;
  return false;
}

bool          
os_socket_connect()
{
  invalid_code_path;
  return false;
}

bool          
os_socket_close()
{
  invalid_code_path;
  return false;
}

Data          
os_socket_recv()
{
  invalid_code_path;
  return (Data){};
}

s32           
os_socket_set_listening()
{
  invalid_code_path;
  return 0;
}

s32           
os_socket_send()
{
  invalid_code_path;
  return 0;
}

s32           
os_socket_send_to(Socket_Handle handle, u32 addr, u32 port, Data data, s32 flags)
{
  invalid_code_path;
  return 0;
}

s32           
os_socket_set_opt(Socket_Handle handle, int level, int option_name, u8* option_value, s32 option_len)
{
  invalid_code_path;
  return 0;
}

#endif // LUMENARIUM_LINUX_NETWORK_H
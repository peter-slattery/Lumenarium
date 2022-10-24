#ifndef LUMENARIUM_LINUX_NETWORK_H
#define LUMENARIUM_LINUX_NETWORK_H 1

Socket_Handle 
os_socket_create(s32 domain, s32 type, s32 protocol)
{
  Socket_Handle result = {};
  OS_SOCKET_TYPE sock = socket(domain, type, protocol);
  if (sock == -1) {
    perror("Error: os_socket_create\n");
    return result;
  }

  result = open_sockets_put(sock);
  if (result.value == 0)
  {
    fprintf(stderr, "Error: os_socket_create = not enough room in open_sockets\n");
  }
  return result;
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
os_socket_close(Socket_Handle handle)
{
  OS_SOCKET_TYPE sock = open_sockets_get(handle);
  close(sock);
  return true;
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
  OS_SOCKET_TYPE sock = open_sockets_get(handle);

  struct sockaddr_in dst = {
    .sin_family = AF_INET,
    .sin_port = hton_u16(port),
    .sin_addr.s_addr = hton_u32(addr),
  };
  struct sockaddr* dst_ptr = (struct sockaddr*)&dst;
  s32 len_sent = sendto(sock, data.base, data.size, flags, dst_ptr, sizeof(struct sockaddr_in));
  if (len_sent == -1)
  {
    perror("Error: os_socket_send_to\n");
    return 0;
  }
  return len_sent;
}

s32           
os_socket_set_opt(Socket_Handle handle, int level, int option_name, u8* option_value, s32 option_len)
{
  OS_SOCKET_TYPE sock = open_sockets_get(handle);
  s32 err = setsockopt(sock, level, option_name, (void*)option_value, (socklen_t)option_len);
  if (err) {
    fprintf(stderr, "Error: setsockopt - %d\n\targs: %d %d %.*s\n", err, level, option_name, option_len, (char*)option_value);
  }
  return 0;
}

#endif // LUMENARIUM_LINUX_NETWORK_H
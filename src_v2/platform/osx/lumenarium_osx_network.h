static Socket_Error os_osx_socket_error_translation_table[] = {
  [EAGAIN] = SocketError_EAGAIN,
  [EBADF] = SocketError_EBADF,
  [ECONNREFUSED] = SocketError_ECONNREFUSED,  
  [EFAULT] = SocketError_EFAULT,  
  [EINTR] = SocketError_EINTR,  
  [EINVAL] = SocketError_EINVAL,  
  [ENOMEM] = SocketError_ENOMEM,  
  [ENOTCONN] = SocketError_ENOTCONN,  
  [ENOTSOCK] = SocketError_ENOTSOCK,  
};

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
    fprintf(stderr, "Error: os_socket_create - not enough room in open_sockets\n");
  }
  return result;
}

bool          
os_socket_bind(Socket_Handle socket, u32 port)
{
  OS_SOCKET_TYPE sock = open_sockets_get(socket);
  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_addr.s_addr = INADDR_ANY,
    .sin_port = htons(port),
  };
  s32 bind_res = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
  if (bind_res < 0) {
    printf("ERROR: os_socket_bind - %d\n", bind_res);
    return false;
  }

  return true;
}

bool          
os_socket_connect()
{
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
  return (Data){};
}

Data          
os_socket_recvfrom(Socket_Handle handle, u8* buffer, u32 buffer_size, Socket_Error* err_out)
{
  OS_SOCKET_TYPE sock = open_sockets_get(handle);
  
  Data result = {
    .base = buffer,
    .size = buffer_size,
  };

  struct sockaddr from = {};
  s32 from_len = sizeof(from);
  
  // TODO: Look into MSG_PEEK - there might be a way to determine
  // the size of the packet without losing it, even on UDP/SOCK_DGRAM
  // connections
  s32 flags = 0;
  
  s32 r = recvfrom(
    sock, 
    result.base, 
    result.size, 
    flags,
    &from,
    (socklen_t*)&from_len
  );
  if (r < 0) {
    if (err_out) {
      *err_out = os_osx_socket_error_translation_table[errno];
    } else {
      printf("UNHANDLED ERROR: os_socket_recvfrom\n\t");
      switch (errno)
      {
        case EAGAIN:       printf("EAGAIN\n"); break;
        case EBADF:        printf("EBADF\n"); break;
        case ECONNREFUSED: printf("ECONNREFUSED\n"); break;
        case EFAULT:       printf("EFAULT\n"); break;
        case EINTR:        printf("EINTR\n"); break;
        case EINVAL:       printf("EINVAL\n"); break;
        case ENOMEM:       printf("ENOMEM\n"); break;
        case ENOTCONN:     printf("ENOTCONN\n"); break;
        case ENOTSOCK:     printf("ENOTSOCK\n"); break;
        default: { printf("%d\n", errno); } break;
      }
    }
  }
  
  result.size = r;
  return result;
}

s32           
os_socket_set_listening()
{
  return 0;
}

s32           
os_socket_send()
{
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

#define PRINT_EVERY_SEND_ADDR 0
#if PRINT_EVERY_SEND_ADDR
  printf("Sending To:\n\tFamily: %d\n\tPort: %d\n\tAddr: %d\n",
    dst.sin_family, dst.sin_port, dst.sin_addr.s_addr
  );
#endif

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
os_socket_set_opt(Socket_Handle handle, int level, int option_name,
u8* option_value, s32 option_len)
{
  OS_SOCKET_TYPE sock = open_sockets_get(handle);
  s32 err = setsockopt(sock, level, option_name, (void*)option_value, (socklen_t)option_len);
  if (err) {
    fprintf(stderr, "Error: setsockopt - %d\n\targs: %d %d %.*s\n", err, level, option_name, option_len, (char*)option_value);
  }
  return 0;
}

internal void incenter_handle_interface_msg(Incenter_State* ins, Data msg);

void
incenter_interface_connection_cleanup(Incenter_State* ins)
{
  if (ins->interface_socket.value == 0) return;
  
  if (!os_socket_close(ins->interface_socket)) {
    printf("Error closing interface connection socket\n");
  }
  ins->interface_socket = (Socket_Handle){0};  
  printf("Incenter Interface Socket Connection Closed.\n");
}

Thread_Result
incenter_interface_connection_thread_proc(u8* user_data)
{
  Incenter_State* ins = (Incenter_State*)user_data;
  
  // Open Socket Connection To Server
  ins->interface_socket = os_socket_create(
      AF_INET, // IPv4
    SOCK_DGRAM,
    0
  );
  
  // Bind the socket to the correct address
  if (!os_socket_bind(ins->interface_socket, 1337)) {
    printf("ERROR: Unable to bind incenter interface socket\n");
    return (Thread_Result){ 0 };
  }
  
  // Message Recv Loop
  while (ins->running)
  {
    u32 next_msg_i = ins->interface_messages_write_next;
    Data* next_msg = ins->interface_messages + next_msg_i;
    next_msg->size = INTERFACE_MESSAGE_SIZE; // reset buffer of known size
    
    Socket_Error err = SocketError_NOERROR;
    *next_msg = os_socket_recvfrom(ins->interface_socket, next_msg->base, next_msg->size, &err);
    if (err == SocketError_EBADF && ins->running) {
      if (!os_socket_close(ins->interface_socket)) {
        printf("Trying to close interface socket before reopening. Socket was already closed.\n");
      }
      ins->interface_socket = os_socket_create(AF_INET, SOCK_DGRAM, 0);
    }
    
    if (err == SocketError_NOERROR) {
      next_msg->base[next_msg->size] = 0;
      // write_next isn't incremented until here because the message
      // is not ready to be read until this point
      ins->interface_messages_write_next = (ins->interface_messages_write_next + 1) % INTERFACE_MESSAGES_CAP;
    }
  }
  
  // Close Down once Incenter indicates it's time to stop running
  incenter_interface_connection_cleanup(ins);
  
  return (Thread_Result){ 0 };
}

internal void
incenter_interface_connection_init(App_State* state, Incenter_State* ins)
{
  // Start Thread to handle blocking socket calls
  ins->interface_thread = os_thread_begin(incenter_interface_connection_thread_proc, (u8*)ins);
  
  for (u32 msg_i = 0; msg_i < INTERFACE_MESSAGES_CAP; msg_i++)
  {
    ins->interface_messages[msg_i] = (Data){
      .base = allocator_alloc(permanent, INTERFACE_MESSAGE_SIZE),
      .size = INTERFACE_MESSAGE_SIZE,
    };
  }
}

internal void
incenter_interface_connection_frame(App_State* state, Incenter_State* ins)
{
  while (ins->interface_messages_write_next != ins->interface_messages_read_next)
  {
    u32 read_next = ins->interface_messages_read_next;
    ins->interface_messages_read_next = (read_next + 1) % INTERFACE_MESSAGES_CAP;
    
    Data msg_read = ins->interface_messages[read_next];
    printf("Handling message on the main thread\n");
    printf("  %s\n", (char*)msg_read.base);
    
    incenter_handle_interface_msg(ins, msg_read);
  }
}

#define INCENTER_CMP3(base, str) ((base[0] == str[0]) && (base[1] == str[1]) && (base[2] == str[2]))

internal void
incenter_handle_sliding_scale_msg(Incenter_State* ins, Incenter_Scene scene, char msg)
{
  assert(scene.kind == Incenter_SceneKind_SlidingScale);
  if (msg != 'M' && msg != 'L' && msg != 'C') {
    printf("Invalid input sent to sliding scale scene: %c\n", msg);
    printf("  Scene: %s\n", scene.name);
    return;
  }
  
  switch (msg) {
    case 'M': {
      ins->input_pct += 0.1f;
    } break;
    
    case 'L': {
      ins->input_pct -= 0.1f;
    } break;
    
    case 'C': {
      live_answers_input_r32(ins, scene, ins->input_pct);
      ins->scene_mode = Incenter_SceneMode_Passive;
    } break;
  }
  
  ins->input_pct = clamp(0, ins->input_pct, 1);
}

internal void
incenter_handle_yes_no_msg(Incenter_State* ins, Incenter_Scene scene, char msg)
{
  assert(scene.kind == Incenter_SceneKind_YesOrNo);
  if (msg != 'Y' && msg != 'N') {
    printf("Invalid input sent to yes no scene: %c\n", msg);
    printf("  Scene: %s\n", scene.name);
    return;
  }
  
  switch (msg) {
    case 'Y': {
      ins->input_option = 1;
    } break;
    
    case 'N': {
      ins->input_option = 0;
    } break;
  }
  
  live_answers_input_u32(ins, scene, ins->input_option);
  ins->scene_mode = Incenter_SceneMode_Passive;
}

internal void
incenter_handle_three_option_msg(Incenter_State* ins, Incenter_Scene scene, char msg)
{
  assert(scene.kind == Incenter_SceneKind_ThreeOption);
  if (msg != '1' && msg != '2' && msg != '3') {
    printf("Invalid input sent to 3 option scene: %c\n", msg);
    printf("  Scene: %s\n", scene.name);
    return;
  }
  
  ins->input_option = (msg - '0') - 1;
  
  live_answers_input_u32(ins, scene, ins->input_option);
  ins->scene_mode = Incenter_SceneMode_Passive;
}

internal void
incenter_handle_interface_msg(Incenter_State* ins, Data msg)
{
  u32 msg_kind = Incenter_InterfaceMessage_Invalid;
  u32 scene_id = 0;
  if (INCENTER_CMP3(msg.base, "UIN")) {
    msg_kind = Incenter_InterfaceMessage_UserInput;
  } else if (INCENTER_CMP3(msg.base, "GTS")){
    msg_kind = Incenter_InterfaceMessage_GoToScene;
    
    char* scene_id_start = (char*)(msg.base + 3);
    scene_id = scene_id_start[0] - '0';
    if (scene_id_start[1] != 0) {
      scene_id *= 10;
      scene_id += scene_id_start[1] - '0';
    }
  }
  
  switch (msg_kind)
  {
    case Incenter_InterfaceMessage_GoToScene:
    {
      if (scene_id > Incenter_Scene_Invalid &&
          scene_id < Incenter_Scene_Count)
      {
        if (scene_id != ins->scene_at) {
          incenter_scene_go_to(ins, scene_id);
        }
      }
      else
      {
        printf("Unknown Scene ID Requested: %d\n", scene_id);
      }      
    } break;
    
    case Incenter_InterfaceMessage_UserInput:
    {
      Incenter_Scene scene = ins->scenes[ins->scene_at];
      char input_kind = msg.base[3];
      switch (scene.kind) {
        case Incenter_SceneKind_SlidingScale: {
          incenter_handle_sliding_scale_msg(ins, scene, input_kind);
        } break;
        
        case Incenter_SceneKind_YesOrNo: {
          incenter_handle_yes_no_msg(ins, scene, input_kind);
        } break;
        
        case Incenter_SceneKind_ThreeOption: {
          incenter_handle_three_option_msg(ins, scene, input_kind);
        } break;
      }
    } break;
    
    default:
    {
      printf("Unknown message kind: %d\n", msg_kind);
      // TODO: Print the actual message received      
    } break;
  }
}
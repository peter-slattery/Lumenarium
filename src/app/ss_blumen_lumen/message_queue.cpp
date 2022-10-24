
internal void
MessageQueue_Init(blumen_network_msg_queue* Queue, gs_memory_arena* Arena)
{
  gs_data MessageQueueData = PushSize(Arena, DEFAULT_QUEUE_ENTRY_SIZE * BLUMEN_MESSAGE_QUEUE_COUNT);
  gs_memory_cursor C = MemoryCursorCreateFromData(MessageQueueData);
  
  for (u32 i = 0; i < BLUMEN_MESSAGE_QUEUE_COUNT; i++)
  {
    Queue->Buffers[i] = MemoryCursorPushSize(&C, DEFAULT_QUEUE_ENTRY_SIZE);
  }
}

internal bool
MessageQueue_CanWrite(blumen_network_msg_queue Queue)
{
  bool Result = ((Queue.WriteHead >= Queue.ReadHead) ||
                 (Queue.WriteHead < Queue.ReadHead));
  return Result;
}

internal bool
MessageQueue_Write(blumen_network_msg_queue* Queue, gs_data Msg)
{
  Assert(Msg.Size <= DEFAULT_QUEUE_ENTRY_SIZE);
  
  u32 Index = Queue->WriteHead;
  Assert(Index >= 0 && 
         Index < BLUMEN_MESSAGE_QUEUE_COUNT);
  
  gs_data* Dest = Queue->Buffers + Index;
  CopyMemoryTo(Msg.Memory, Dest->Memory, Msg.Size);
  Dest->Size = Msg.Size;
  
  // NOTE(pjs): We increment write head at the end of writing so that
  // a reader thread doesn't pull the message off before we've finished
  // filling it out
  Queue->WriteHead = (Queue->WriteHead + 1) % BLUMEN_MESSAGE_QUEUE_COUNT;
  return true;
}

internal bool
MessageQueue_CanRead(blumen_network_msg_queue Queue)
{
  bool Result = (Queue.ReadHead != Queue.WriteHead);
  return Result;
}

internal gs_data
MessageQueue_Peek(blumen_network_msg_queue* Queue)
{
  gs_data Result = {};
  u32 ReadIndex = Queue->ReadHead;
  if (ReadIndex >= BLUMEN_MESSAGE_QUEUE_COUNT)
  {
    ReadIndex = 0;
  }
  Result = Queue->Buffers[ReadIndex];
  return Result;
}

internal gs_data
MessageQueue_Read(blumen_network_msg_queue* Queue)
{
  gs_data Result = {};
  u32 ReadIndex = Queue->ReadHead++;
  if (ReadIndex >= BLUMEN_MESSAGE_QUEUE_COUNT)
  {
    Queue->ReadHead = 0;
    ReadIndex = 0;
  }
  Result = Queue->Buffers[ReadIndex];
  return Result;
}

internal void
MessageQueue_Clear(blumen_network_msg_queue* Queue)
{
  while (MessageQueue_CanRead(*Queue))
  {
    MessageQueue_Read(Queue);
  }
}

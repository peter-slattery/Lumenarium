
internal void
register_output_method(Output* output, Output_Data_Kind kind, Output_Method_Update* update, Build_Output_Data_Buffer* proc, u8* method_data)
{
  output->methods.update_procs[kind] = update;
  output->methods.procs[kind] = proc;
  output->methods.method_data[kind] = method_data;
}

internal Output_Data* 
output_data_queue_push(Output_Data_Queue* q, u32 size, Output_Data_Kind kind)
{
  Output_Data* d = allocator_alloc_struct(q->a, Output_Data);
  d->kind = kind;
  d->data.size = size;
  d->data.base = allocator_alloc(q->a, size);
  sll_push(q->first, q->last, d);
  return d;
}

internal void
output_data_set_network_addr(Output_Data* d, u32 send_addr, u32 port)
{
  d->network.v4_addr = send_addr;
  d->network.port = port;
}

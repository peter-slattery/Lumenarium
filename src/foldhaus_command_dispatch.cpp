internal void
InitializeInputCommandRegistry (input_command_registry* CommandRegistry,
                                s32 Size,
                                memory_arena* Storage)
{
    CommandRegistry->Commands = PushArray(Storage, input_command, Size);
    CommandRegistry->Size = Size;
    CommandRegistry->Used = 0;
}

internal input_command*
FindExistingCommand (input_command_registry* CommandRegistry, key_code Key, key_code Mdfr)
{
    input_command* Result = 0;
    
    for (s32 Cmd = 0; Cmd < CommandRegistry->Used; Cmd++)
    {
        input_command* Command = CommandRegistry->Commands + Cmd;
        if (Command->Key == Key && Command->Mdfr == Mdfr)
        {
            Result = Command;
            break;
        }
    }
    
    return Result;
}

internal void
RegisterKeyPressCommand (input_command_registry* CommandRegistry,
                         key_code Key,
                         b32 PersistsUntilReleased,
                         key_code Mdfr,
                         input_command_proc* Proc)
{
    input_command* Command = FindExistingCommand(CommandRegistry, Key, Mdfr);
    
    if (!Command)
    {
        Assert(CommandRegistry->Size > CommandRegistry->Used);
        Assert(Mdfr == KeyCode_Invalid || Mdfr == KeyCode_LeftShift || Mdfr == KeyCode_RightShift ||
               Mdfr == KeyCode_LeftCtrl || Mdfr == KeyCode_RightCtrl || Mdfr == KeyCode_Alt);
        Command = CommandRegistry->Commands + CommandRegistry->Used++;
    }
    
    Command->Key = Key;
    Command->PersistsUntilReleased = PersistsUntilReleased;
    Command->Mdfr = Mdfr;
    Command->Proc = Proc;
}

internal void
RegisterMouseWheelCommand (input_command_registry* CommandRegistry,
                           input_command_proc* Proc)
{
    CommandRegistry->MouseWheelCommand = Proc;
}

internal input_command_queue
InitializeCommandQueue(command_queue_entry* Memory, s32 MemorySize)
{
    input_command_queue Result = {};
    Result.Size = MemorySize;
    Result.Used = 0;
    Result.Commands = Memory;
    return Result;
}

internal void
RemoveNonPersistantCommandsFromQueueAndUpdatePersistentEvents(input_command_queue* Queue)
{
    s32 PersistantCommandsCount = 0;
    for (s32 i = 0; i < Queue->Used; i++)
    {
        command_queue_entry* Entry = Queue->Commands + i;
        if (Entry->Command.PersistsUntilReleased)
        {
            Entry->Event.State |= KeyState_WasDown;
            // NOTE(Peter): If i == PersistantCommandsCount, then we don't need to copy the 
            // command anywhere
            if (i != PersistantCommandsCount)
            {
                Queue->Commands[PersistantCommandsCount] = *Entry;
            }
            PersistantCommandsCount++;
        }
    }
    Queue->Used = PersistantCommandsCount;
}

internal void
PushCommandOnQueue(input_command_queue* Queue, input_command Command, input_entry Event)
{
    Assert(Queue->Used < Queue->Size);
    command_queue_entry Entry = {};
    Entry.Command = Command;
    Entry.Event = Event;
    Queue->Commands[Queue->Used++] = Entry;
}

internal void
RemoveCommandFromQueue(input_command_queue* Queue, input_command Command, input_entry Event)
{
    s32 CommandIndex = 0;
    for (CommandIndex = 0; CommandIndex < Queue->Used; CommandIndex++)
    {
        command_queue_entry* Entry = Queue->Commands + CommandIndex;
        if(Entry->Event.Key == Event.Key)
        {
            break;
        }
    }
    
    // NOTE(Peter): If we made it through the queue without finding an event, there wasn't one
    // to remove. This happens when we've changed command registries as a result of an input command, 
    // and the command exists in the new registry.
    // For example: 
    //   clicking a mouse button triggers a command to switch registries
    //   the new registry tracks mouse drag (persist until release)
    //   when the mouse is released, the event fires, but there is no mouse down event to remove
    // For this reason, I'm allowing the case where we try and remove a command where non exists
    // I don't think this is a great solution but Im not super familiar with the codebase right now
    // so leaving it as is. revisit if it becomes a problem.
    if (CommandIndex < Queue->Used) 
    { 
        Queue->Used -= 1;
        
        for (; CommandIndex < Queue->Used; CommandIndex++)
        {
            Queue->Commands[CommandIndex] = Queue->Commands[CommandIndex + 1];
        }
    }
}

internal void
QueueNextFrameCommandRegistry (input_command_registry* NewRegistry, app_state* State)
{
    State->NextCommandRegistry = NewRegistry;
}

internal void
ActivateQueuedCommandRegistry (app_state* State)
{
    if (State->NextCommandRegistry)
    {
        State->ActiveCommands = State->NextCommandRegistry;
        State->NextCommandRegistry = 0;
    }
}
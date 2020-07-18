//
// File: foldhaus_command_dispatch.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_COMMAND_DISPATCH_H

#define FOLDHAUS_INPUT_COMMAND_PROC(name) void name(app_state* State, input_entry Event, mouse_state Mouse)
typedef FOLDHAUS_INPUT_COMMAND_PROC(input_command_proc);

// NOTE(Peter): Helper function so I don't have to remember the parameters to this define
#define ExecFoldhausCommand(cmd) cmd(State, Event, Mouse)

enum input_command_flags
{
    Command_Began = 1 << 0,
    Command_Held = 1 << 1,
    Command_Ended = 1 << 2,
};

#define Command_Any Command_Began | Command_Held | Command_Ended

// TODO(Peter): At the moment these are all key press commands. Need a way to differentiate between
// press and hold. Probably add a second array to input_command_Registry
struct input_command
{
    key_code Key;
    key_code Mdfr;
    b32 Flags;
    input_command_proc* Proc;
};

struct input_command_registry
{
    input_command* Commands;
    s32 Size;
    s32 Used;
    
    input_command_proc* MouseWheelCommand;
};

struct command_queue_entry
{
    input_command Command;
    input_entry Event;
};

struct input_command_queue
{
    s32 Size;
    s32 Used;
    command_queue_entry* Commands;
};

internal void
InitializeInputCommandRegistry (input_command_registry* CommandRegistry,
                                s32 Size,
                                gs_memory_arena* Storage)
{
    CommandRegistry->Commands = PushArray(Storage, input_command, Size);
    CommandRegistry->Size = Size;
    CommandRegistry->Used = 0;
}

internal void
RegisterMouseWheelCommand (input_command_registry* CommandRegistry,
                           input_command_proc* Proc)
{
    CommandRegistry->MouseWheelCommand = Proc;
}

internal s32
GetCommandIndexInQueue(input_command_queue* Queue, input_command Command, input_entry Event)
{
    s32 Result = -1;
    for (s32 CommandIndex = 0; CommandIndex < Queue->Used; CommandIndex++)
    {
        command_queue_entry* Entry = Queue->Commands + CommandIndex;
        if(Entry->Event.Key == Event.Key)
        {
            Result = CommandIndex;
            break;
        }
    }
    return Result;
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
ClearCommandQueue(input_command_queue* Queue)
{
    Queue->Used = 0;
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
RemoveCommandFromQueue(input_command_queue* Queue, s32 Index)
{
    s32 CommandIndex = Index;
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
RemoveCommandFromQueue(input_command_queue* Queue, input_command Command, input_entry Event)
{
    s32 CommandIndex = GetCommandIndexInQueue(Queue, Command, Event);
    
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
    RemoveCommandFromQueue(Queue, CommandIndex);
}

internal input_command*
FindExistingCommand (input_command_registry CommandRegistry, key_code Key, key_code Mdfr, b32 Flags)
{
    input_command* Result = 0;
    
    for (s32 Cmd = 0; Cmd < CommandRegistry.Used; Cmd++)
    {
        input_command* Command = CommandRegistry.Commands + Cmd;
        if (Command->Key == Key && Command->Mdfr == Mdfr)
        {
            b32 FlagsOverlap = Flags & Command->Flags;
            if (FlagsOverlap)
            {
                Result = Command;
                break;
            }
        }
    }
    
    return Result;
}

internal b32
FindAndPushExistingCommand(input_command_registry CommandRegistry, input_entry Event, b32 Flags, input_command_queue* CommandQueue)
{
    b32 CommandFound = false;
    input_command* Command = FindExistingCommand(CommandRegistry, Event.Key, (key_code)0, Flags);
    if (Command)
    {
        PushCommandOnQueue(CommandQueue, *Command, Event);
        CommandFound = true;
    }
    return CommandFound;
}

internal void
RegisterKeyPressCommand (input_command_registry* CommandRegistry,
                         key_code Key,
                         b32 Flags,
                         key_code Mdfr,
                         input_command_proc* Proc)
{
    input_command* Command = FindExistingCommand(*CommandRegistry, Key, Mdfr, Flags);
    
    if (!Command)
    {
        Assert(CommandRegistry->Size > CommandRegistry->Used);
        Assert(Mdfr == KeyCode_Invalid || Mdfr == KeyCode_LeftShift || Mdfr == KeyCode_RightShift ||
               Mdfr == KeyCode_LeftCtrl || Mdfr == KeyCode_RightCtrl || Mdfr == KeyCode_Alt);
        Command = CommandRegistry->Commands + CommandRegistry->Used++;
    }
    
    Command->Key = Key;
    Command->Flags = Flags;
    Command->Mdfr = Mdfr;
    Command->Proc = Proc;
}

#define FOLDHAUS_COMMAND_DISPATCH_H
#endif // FOLDHAUS_COMMAND_DISPATCH_H
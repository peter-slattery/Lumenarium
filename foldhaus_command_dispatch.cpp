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
                         b32 Held,
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
    Command->Held = Held;
    Command->Mdfr = Mdfr;
    Command->Proc = Proc;
}

internal void
RegisterMouseWheelCommand (input_command_registry* CommandRegistry,
                           input_command_proc* Proc)
{
    CommandRegistry->MouseWheelCommand = Proc;
}

internal void
ExecuteAllRegisteredCommands (input_command_registry* CommandRegistry,
                              input Input,
                              app_state* State)
{
    if (Input.New->MouseScroll != 0)
    {
        CommandRegistry->MouseWheelCommand(State, Input);
    }
    
    for (s32 i = 0; i < CommandRegistry->Used; i++)
    {
        input_command Command = CommandRegistry->Commands[i];
        if (Command.Held)
        {
            if (KeyDown(Input, Command.Key) &&
                (Command.Mdfr == KeyCode_Invalid || KeyDown(Input, Command.Mdfr)))
            {
                Command.Proc(State, Input);
            }
        }
        else
        {
            if (KeyTransitionedDown(Input, Command.Key) && 
                (Command.Mdfr == KeyCode_Invalid || KeyDown(Input, Command.Mdfr)))
            {
                Command.Proc(State, Input);
            }
        }
    }
}

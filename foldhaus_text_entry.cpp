internal void
ResetTextInput (text_input* Input)
{
    Input->CursorPosition = 0;
    Input->Buffer.Length = 0;
}

internal void
PipeSearchStringToDestination (text_input* Input)
{
    switch (Input->Destination.Type)
    {
        case TextTranslateTo_String:
        {
            CopyStringTo(Input->Buffer, Input->Destination.StringDest);
        }break;
        
        case TextTranslateTo_R32:
        {
            parse_result FloatParseResult = ParseFloat(Input->Buffer.Memory, Input->Buffer.Length);
            *Input->Destination.FloatDest = FloatParseResult.FloatValue;
        }break;
        
        InvalidDefaultCase;
    }
}

FOLDHAUS_INPUT_COMMAND_PROC(RemoveCharacterFromEntryString)
{
    if (State->ActiveTextEntry.CursorPosition > 0)
    {
        RemoveCharAt(&State->ActiveTextEntry.Buffer,
                     State->ActiveTextEntry.CursorPosition - 1);
        State->ActiveTextEntry.CursorPosition--;
    }
}

internal void 
SetTextInputDestinationToString (text_input* TextInput, string* DestinationString)
{
    ResetTextInput(TextInput);
    TextInput->Destination.Type = TextTranslateTo_String;
    TextInput->Destination.StringDest = DestinationString;
    CopyStringTo(*DestinationString, &TextInput->Buffer);
}

internal void
SetTextInputDestinationToFloat (text_input* TextInput, r32* DestinationFloat)
{
    ResetTextInput(TextInput);
    TextInput->Destination.Type = TextTranslateTo_R32;
    TextInput->Destination.FloatDest = DestinationFloat;
    PrintF(&TextInput->Buffer, "%f", *DestinationFloat);
    
    if (*DestinationFloat == 0.0f)
    {
        TextInput->CursorPosition = 1;
    }
}

internal void
AppendInputToEntryString (text_input* EntryString, char* InputString, s32 InputStringLength)
{
    if (InputStringLength > 0)
    {
        for (s32 i = 0; i < InputStringLength; i++)
        {
            InsertChar(&EntryString->Buffer, InputString[i], EntryString->CursorPosition);
            EntryString->CursorPosition++;
        }
    }
    PipeSearchStringToDestination(EntryString);
}

FOLDHAUS_INPUT_COMMAND_PROC(TextEntryMoveCursorRight)
{
    State->ActiveTextEntry.CursorPosition = GSMin(State->ActiveTextEntry.Buffer.Length, 
                                                  State->ActiveTextEntry.CursorPosition + 1);
}

FOLDHAUS_INPUT_COMMAND_PROC(TextEntryMoveCursorLeft)
{
    State->ActiveTextEntry.CursorPosition = GSMax(0, 
                                                  State->ActiveTextEntry.CursorPosition - 1);
}

FOLDHAUS_INPUT_COMMAND_PROC(LeaveTextEntryMode)
{
    // TODO(Peter): Make this more flexible. Should return to whatever came before
    State->ActiveCommands = &State->InputCommandRegistry;
}

internal void
InitializeTextInputCommands (input_command_registry* SearchCommands, memory_arena* PermanentStorage)
{
    if (SearchCommands->Size > 0)
    {
        RegisterKeyPressCommand(SearchCommands, KeyCode_Backspace, false, KeyCode_Invalid, RemoveCharacterFromEntryString);
        RegisterKeyPressCommand(SearchCommands, KeyCode_LeftArrow, false, KeyCode_Invalid, TextEntryMoveCursorLeft);
        RegisterKeyPressCommand(SearchCommands, KeyCode_RightArrow, false, KeyCode_Invalid, TextEntryMoveCursorRight);
        RegisterKeyPressCommand(SearchCommands, KeyCode_Enter, false, KeyCode_Invalid,
                                LeaveTextEntryMode);
    }
}

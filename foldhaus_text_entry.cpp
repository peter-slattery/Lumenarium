internal void
ResetTextInput (text_entry* Input)
{
    Input->CursorPosition = 0;
    Input->Buffer.Length = 0;
}

internal void
PipeSearchStringToDestination (text_entry* Input)
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

internal void
RemoveCharacterAtCursor (text_entry* TextEntry)
{
    if (TextEntry->CursorPosition > 0)
    {
        RemoveCharAt(&TextEntry->Buffer,
                     TextEntry->CursorPosition - 1);
        TextEntry->CursorPosition--;
    }
}

internal void 
SetTextInputDestinationToString (text_entry* TextInput, string* DestinationString)
{
    ResetTextInput(TextInput);
    TextInput->Destination.Type = TextTranslateTo_String;
    TextInput->Destination.StringDest = DestinationString;
    CopyStringTo(*DestinationString, &TextInput->Buffer);
}

internal void
SetTextInputDestinationToFloat (text_entry* TextInput, r32* DestinationFloat)
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
AppendInputToEntryString (text_entry* EntryString, char* InputString, s32 InputStringLength)
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

internal void
MoveCursorRight (text_entry* TextEntry)
{
    TextEntry->CursorPosition = GSMin(TextEntry->Buffer.Length, 
                                      TextEntry->CursorPosition + 1);
}

internal void
MoveCursorLeft (text_entry* TextEntry)
{
    TextEntry->CursorPosition = GSMax(0, TextEntry->CursorPosition - 1);
}

FOLDHAUS_INPUT_COMMAND_PROC(RemoveCharacterFromEntryString)
{
    RemoveCharacterAtCursor(&State->ActiveTextEntry);
}

FOLDHAUS_INPUT_COMMAND_PROC(TextEntryMoveCursorRight)
{
    MoveCursorRight(&State->ActiveTextEntry);
}

FOLDHAUS_INPUT_COMMAND_PROC(TextEntryMoveCursorLeft)
{
    MoveCursorLeft(&State->ActiveTextEntry);
}

internal void
InitializeTextInputCommands (input_command_registry* Commands, memory_arena* PermanentStorage)
{
    if (Commands->Size > 0)
    {
        RegisterKeyPressCommand(Commands, KeyCode_Backspace, false, KeyCode_Invalid, RemoveCharacterFromEntryString);
        RegisterKeyPressCommand(Commands, KeyCode_LeftArrow, false, KeyCode_Invalid, TextEntryMoveCursorLeft);
        RegisterKeyPressCommand(Commands, KeyCode_RightArrow, false, KeyCode_Invalid, TextEntryMoveCursorRight);
    }
}

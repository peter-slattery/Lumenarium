FOLDHAUS_INPUT_COMMAND_PROC(RemoveCharacterFromEntryString)
{
    if (State->GeneralPurposeSearch.CursorPosition > 0)
    {
        RemoveCharAt(&State->GeneralPurposeSearch.Buffer,
                     State->GeneralPurposeSearch.CursorPosition - 1);
        State->GeneralPurposeSearch.CursorPosition--;
    }
}

internal void 
ActivateTextEntry(text_input* ActiveEntryString, app_state* State)
{
    State->ActiveTextEntry = ActiveEntryString;
    State->ActiveTextEntry->PreviousCommandRegistry = State->ActiveCommands;
    State->ActiveCommands = &State->TextEntryCommandRegistry;
}

internal void 
DeactivateTextEntry(app_state* State)
{
    if (State->ActiveTextEntry->PreviousCommandRegistry != 0)
    {
        State->ActiveCommands = State->ActiveTextEntry->PreviousCommandRegistry;
        State->ActiveTextEntry = 0;
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
}

FOLDHAUS_INPUT_COMMAND_PROC(TextEntryMoveCursorRight)
{
    State->ActiveTextEntry->CursorPosition = GSMin(State->ActiveTextEntry->Buffer.Length, 
                                                   State->ActiveTextEntry->CursorPosition + 1);
}

FOLDHAUS_INPUT_COMMAND_PROC(TextEntryMoveCursorLeft)
{
    State->ActiveTextEntry->CursorPosition = GSMax(0, 
                                                   State->ActiveTextEntry->CursorPosition - 1);
}

internal void
InitializeTextInputCommands (input_command_registry* SearchCommands, memory_arena* PermanentStorage)
{
    if (SearchCommands->Size > 0)
    {
        RegisterKeyPressCommand(SearchCommands, KeyCode_Backspace, false, KeyCode_Invalid, RemoveCharacterFromEntryString);
        RegisterKeyPressCommand(SearchCommands, KeyCode_LeftArrow, false, KeyCode_Invalid, TextEntryMoveCursorLeft);
        RegisterKeyPressCommand(SearchCommands, KeyCode_RightArrow, false, KeyCode_Invalid, TextEntryMoveCursorRight);
    }
}

//
// File: foldhaus_text_entry.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_TEXT_ENTRY_CPP

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
            parse_result FloatParseResult = ParseFloat(StringExpand(Input->Buffer));
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

FOLDHAUS_INPUT_COMMAND_PROC(TextEntryInsertChar)
{
    
    char Char = CharacterFromKeyCode(Event.Key);
    if (Event.Key >= KeyCode_A && Event.Key <= KeyCode_Z && (Event.Modifiers & Modifier_Shift) == 0)
    {
        Char += ('a' - 'A');
    }
    
    InsertChar(&State->ActiveTextEntry.Buffer, Char, State->ActiveTextEntry.CursorPosition);
    State->ActiveTextEntry.CursorPosition++;
    PipeSearchStringToDestination(&State->ActiveTextEntry);
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
        RegisterKeyPressCommand(Commands, KeyCode_Backspace, Command_Began | Command_Held, KeyCode_Invalid, RemoveCharacterFromEntryString);
        RegisterKeyPressCommand(Commands, KeyCode_LeftArrow, Command_Began | Command_Held, KeyCode_Invalid, TextEntryMoveCursorLeft);
        RegisterKeyPressCommand(Commands, KeyCode_RightArrow, Command_Began | Command_Held, KeyCode_Invalid, TextEntryMoveCursorRight);
        
        for (s32 i = KeyCode_a; i < KeyCode_UpArrow; i++)
        {
            RegisterKeyPressCommand(Commands, (key_code)i, Command_Began | Command_Held, KeyCode_Invalid, TextEntryInsertChar);
        }
        
    }
}

#define DEFAULT_TEXT_ENTRY_INPUT_COMMANDS_ARRAY_ENTRY \
{ KeyCode_Backspace, KeyCode_Invalid, Command_Began | Command_Held, RemoveCharacterFromEntryString }, \
{ KeyCode_LeftArrow, KeyCode_Invalid, Command_Began | Command_Held, TextEntryMoveCursorLeft }, \
{ KeyCode_RightArrow, KeyCode_Invalid, Command_Began | Command_Held, TextEntryMoveCursorRight }, \
{ KeyCode_a, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_b, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_c, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_d, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_e, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_f, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_g, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_h, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_i, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_j, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_k, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_l, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_m, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_n, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_o, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_p, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_q, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_r, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_s, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_t, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_u, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_v, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_w, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_x, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_y, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_z, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_A, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_B, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_C, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_D, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_E, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_F, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_G, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_H, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_I, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_J, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_K, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_L, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_M, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_N, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_O, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_P, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Q, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_R, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_S, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_T, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_U, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_V, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_W, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_X, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Y, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Z, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_0, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_1, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_2, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_3, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_4, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_5, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_6, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_7, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_8, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_9, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Num0, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Num1, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Num2, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Num3, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Num4, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Num5, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Num6, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Num7, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Num8, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Num9, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Bang, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_At, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Pound, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Dollar, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Percent, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Carrot, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Ampersand, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Star, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_LeftParen, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_RightParen, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Minus, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Plus, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Equals, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Underscore, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_LeftBrace, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_RightBrace, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_LeftBracket, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_RightBracket, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Colon, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_SemiColon, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_SingleQuote, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_DoubleQuote, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_ForwardSlash, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Backslash, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Pipe, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Comma, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Period, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_QuestionMark, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_LessThan, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_GreaterThan, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_Tilde, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }, \
{ KeyCode_BackQuote, KeyCode_Invalid, Command_Began | Command_Held, TextEntryInsertChar }


#define FOLDHAUS_TEXT_ENTRY_CPP
#endif // FOLDHAUS_TEXT_ENTRY_CPP
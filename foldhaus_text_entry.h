struct text_input
{
    char* Backbuffer;
    string Buffer;
    s32 CursorPosition;
    input_command_registry* PreviousCommandRegistry;
};
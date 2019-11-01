#if USAGE_CODE

RegisterInputCommand(KeyCode_UpArrow, SACNView_DrawNextUniverse, Context);
RegisterInputCommand(KeyCode_MouseLeftButton, PanCamera, Context);

ExecuteRegisteredCommands(Context, Input);

#endif // USAGE_CODE

#define FOLDHAUS_INPUT_COMMAND_PROC(name) void name(app_state* State, input_entry Event, mouse_state Mouse)
typedef FOLDHAUS_INPUT_COMMAND_PROC(input_command_proc);

// TODO(Peter): At the moment these are all key press commands. Need a way to differentiate between
// press and hold. Probably add a second array to input_command_Registry
struct input_command
{
    key_code Key;
    b32 PersistsUntilReleased;
    key_code Mdfr;
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
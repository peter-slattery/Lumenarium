#define FOLDHAUS_INPUT_COMMAND_PROC(name) void name(app_state* State, input_entry Event, mouse_state Mouse)
typedef FOLDHAUS_INPUT_COMMAND_PROC(input_command_proc);

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
    b32 Flags;
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
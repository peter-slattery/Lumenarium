typedef struct operation_mode operation_mode;

#define OPERATION_STATE_DEF(name) struct name

#define OPERATION_RENDER_PROC(name) void name(app_state* State, render_command_buffer* RenderBuffer, operation_mode Operation,  mouse_state Mouse)
typedef OPERATION_RENDER_PROC(operation_render_proc);

struct operation_mode
{
    input_command_registry Commands;
    operation_render_proc* Render;
    u8* OpStateMemory;
};

#define OPERATION_MODES_MAX 32
struct operation_mode_system
{
    s32 ActiveModesCount;
    operation_mode ActiveModes[OPERATION_MODES_MAX];
    arena_snapshot ModeMemorySnapshots[OPERATION_MODES_MAX];
    
    // NOTE(Peter): This acts as mode scoped memory. When a mode gets activated, it can allocate
    // temporary memory which then gets freed when the mode is deactivated
    memory_arena Arena;
};

internal operation_mode*
ActivateOperationMode (operation_mode_system* System, operation_render_proc* RenderProc)
{
    operation_mode* Result = 0;
    
    Assert(System->ActiveModesCount < OPERATION_MODES_MAX);
    s32 ModeIndex = System->ActiveModesCount++;
    
    System->ModeMemorySnapshots[ModeIndex] = TakeSnapshotOfArena(&System->Arena);
    
    operation_mode NewMode = {};
    System->ActiveModes[ModeIndex] = NewMode;
    
    Result = &System->ActiveModes[ModeIndex];
    Result->Render = RenderProc;
    return Result;
}

#define ActivateOperationModeWithCommands(sys, cmds, render) \
ActivateOperationModeWithCommands_(sys, cmds, (s32)(sizeof(cmds) / sizeof(cmds[0])), render);

internal operation_mode*
ActivateOperationModeWithCommands_(operation_mode_system* System, input_command* Commands, s32 CommandsCount, operation_render_proc* RenderProc)
{
    operation_mode* NewMode = ActivateOperationMode(System, RenderProc);
    
    InitializeInputCommandRegistry(&NewMode->Commands, CommandsCount, &System->Arena);
    for (s32 i = 0; i < CommandsCount; i++)
    {
        input_command Command = Commands[i];
        RegisterKeyPressCommand(&NewMode->Commands, Command.Key, Command.Flags, Command.Mdfr, Command.Proc);
    }
    
    return NewMode;
}

internal void
DeactivateCurrentOperationMode (operation_mode_system* System)
{
    Assert(System->ActiveModesCount > 0);
    s32 ModeIndex = --System->ActiveModesCount;
    ClearArenaToSnapshot(&System->Arena, System->ModeMemorySnapshots[ModeIndex]);
}

#define CreateOperationState(mode, modeSystem, stateType) \
(stateType*)CreateOperationState_(mode, modeSystem, sizeof(stateType))

#define GetCurrentOperationState(modeSystem, stateType) \
(stateType*)State->Modes.ActiveModes[State->Modes.ActiveModesCount - 1].OpStateMemory;


internal u8*
CreateOperationState_ (operation_mode* Mode, operation_mode_system* System, s32 StateSize)
{
    Mode->OpStateMemory = PushSize(&System->Arena, StateSize);
    return Mode->OpStateMemory;
}

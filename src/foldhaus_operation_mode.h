typedef struct operation_mode operation_mode;

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
ActivateOperationMode (operation_mode_system* System)
{
    Assert(System->ActiveModesCount < OPERATION_MODES_MAX);
    s32 ModeIndex = System->ActiveModesCount++;
    System->ActiveModes[ModeIndex] = {};
    System->ModeMemorySnapshots[ModeIndex] = TakeSnapshotOfArena(System->Arena);
    return &System->ActiveModes[ModeIndex];
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

internal u8*
CreateOperationState_ (operation_mode* Mode, operation_mode_system* System, s32 StateSize)
{
    Mode->OpStateMemory = PushSize(&System->Arena, StateSize);
    return Mode->OpStateMemory;
}
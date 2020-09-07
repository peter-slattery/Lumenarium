//
// File: foldhaus_operation_mode.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_OPERATION_MODE_H

typedef struct operation_mode operation_mode;

#define OPERATION_STATE_DEF(name) struct name

#define OPERATION_RENDER_PROC(name) void name(app_state* State, render_command_buffer* RenderBuffer, operation_mode Operation,  mouse_state Mouse, context Context)
typedef OPERATION_RENDER_PROC(operation_render_proc);

struct operation_mode
{
    input_command_registry Commands;
    operation_render_proc* Render;
    gs_memory_cursor Memory;
    u8* OpStateMemory;
};

#define OPERATION_MODES_MAX 32
struct operation_mode_system
{
    s32 ActiveModesCount;
    operation_mode ActiveModes[OPERATION_MODES_MAX];
    //arena_snapshot ModeMemorySnapshots[OPERATION_MODES_MAX];
    gs_data_array ModeMemoryPagesFreeList;
    
    // NOTE(Peter): This acts as mode scoped memory. When a mode gets activated, it can allocate
    // temporary memory which then gets freed when the mode is deactivated
    gs_memory_arena Arena;
};

internal operation_mode_system
OperationModeSystemInit(gs_memory_arena* Storage, gs_thread_context ThreadContext)
{
    operation_mode_system Result = {0};
    // TODO(Peter): Do we really need an arena? Can this just operate in constant memory footprint?
    Result.Arena.Allocator = ThreadContext.Allocator;
    
    Result.ModeMemoryPagesFreeList.CountMax = 16; // TODO(Peter): Static number of modes that can be active simultaneously
    Result.ModeMemoryPagesFreeList.Data = PushArray(Storage, gs_data, Result.ModeMemoryPagesFreeList.CountMax);
    for (u32 Page = 0; Page < Result.ModeMemoryPagesFreeList.CountMax; Page++)
    {
        // TODO(Peter): 4k pages = page size on windows
        Result.ModeMemoryPagesFreeList.Data[Page] = PushSizeToData(Storage, KB(4));
    }
    Result.ModeMemoryPagesFreeList.Count = Result.ModeMemoryPagesFreeList.CountMax;
    
    return Result;
}

internal gs_data
OperationModeTakeMemoryPage(operation_mode_system* System)
{
    Assert(System->ModeMemoryPagesFreeList.Count > 0);
    gs_data Result = {0};
    System->ModeMemoryPagesFreeList.Count -= 1;
    u64 LastIndex = System->ModeMemoryPagesFreeList.Count;
    Result = System->ModeMemoryPagesFreeList.Data[LastIndex];
    return Result;
}

internal void
OperationModeFreeMemoryPage(operation_mode_system* System, gs_data Data)
{
    Assert(System->ModeMemoryPagesFreeList.Count < System->ModeMemoryPagesFreeList.CountMax);
    u64 LastIndex = System->ModeMemoryPagesFreeList.Count;
    System->ModeMemoryPagesFreeList.Count += 1;
    System->ModeMemoryPagesFreeList.Data[LastIndex] = Data;
}

internal operation_mode*
ActivateOperationMode (operation_mode_system* System, operation_render_proc* RenderProc)
{
    Assert(System->ActiveModesCount < OPERATION_MODES_MAX);
    operation_mode* Result = 0;
    s32 ModeIndex = System->ActiveModesCount++;
    
    //System->ModeMemorySnapshots[ModeIndex] = TakeSnapshotOfArena(&System->Arena);
    
    Result = &System->ActiveModes[ModeIndex];
    Result->Memory = CreateMemoryCursor(OperationModeTakeMemoryPage(System));
    Result->Render = RenderProc;
    
    return Result;
}

#define ActivateOperationModeWithCommands(sys, cmds, render) \
ActivateOperationModeWithCommands_(sys, cmds, (s32)(sizeof(cmds) / sizeof(cmds[0])), render);

internal operation_mode*
ActivateOperationModeWithCommands_(operation_mode_system* System, input_command* Commands, s32 CommandsCount, operation_render_proc* RenderProc)
{
    operation_mode* NewMode = ActivateOperationMode(System, RenderProc);
    
#if 0
    InitializeInputCommandRegistry(&NewMode->Commands, CommandsCount, &System->Arena);
    for (s32 i = 0; i < CommandsCount; i++)
    {
        input_command Command = Commands[i];
        RegisterKeyPressCommand(&NewMode->Commands, Command.Key, Command.Flags, Command.Mdfr, Command.Proc);
    }
#else
    NewMode->Commands.Commands = Commands;
    NewMode->Commands.Size = CommandsCount;
    NewMode->Commands.Used = CommandsCount;
#endif
    return NewMode;
}

internal void
DeactivateCurrentOperationMode (operation_mode_system* System)
{
    Assert(System->ActiveModesCount > 0);
    s32 ModeIndex = --System->ActiveModesCount;
    OperationModeFreeMemoryPage(System, System->ActiveModes[ModeIndex].Memory.Data);
    //ClearArenaToSnapshot(&System->Arena, System->ModeMemorySnapshots[ModeIndex]);
}

#define CreateOperationState(mode, modeSystem, stateType) \
(stateType*)CreateOperationState_(mode, modeSystem, sizeof(stateType))

#define GetCurrentOperationState(modeSystem, stateType) \
(stateType*)(modeSystem).ActiveModes[(modeSystem).ActiveModesCount - 1].OpStateMemory;


internal u8*
CreateOperationState_ (operation_mode* Mode, operation_mode_system* System, s32 StateSize)
{
    // NOTE(Peter): This isn't a problem if this fires, it just means our page size is too small,
    // and its time to make the pages dynamically sized
    Assert(Mode->Memory.Data.Size >= StateSize);
    u8* Result = PushSizeOnCursor(&Mode->Memory, StateSize).Memory;
    Mode->OpStateMemory = Result;
    return Result;
}


#define FOLDHAUS_OPERATION_MODE_H
#endif // FOLDHAUS_OPERATION_MODE_H
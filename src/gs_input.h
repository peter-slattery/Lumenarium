enum key_state_flags
{
    KeyState_WasDown = 1 << 0,
    KeyState_IsDown = 1 << 1,
};

#define KeyWasDown(event) ((event.State & KeyState_WasDown) > 0)
#define KeyIsDown(event) ((event.State & KeyState_IsDown) > 0)

struct input_entry
{
    key_code Key;
    b32 State;
    b32 Modifiers;
};

struct input_queue
{
    s32 QueueSize;
    s32 QueueUsed;
    input_entry* Entries;
};

struct mouse_state
{
    s32 Scroll;
    
    v2 Pos;
    v2 OldPos;
    v2 DeltaPos;
    v2 DownPos;
    
    b32 LeftButtonTransitionedDown;
    b32 LeftButtonTransitionedUp;
    b32 MiddleButtonTransitionedDown;
    b32 MiddleButtonTransitionedUp;
    b32 RightButtonTransitionedDown;
    b32 RightButtonTransitionedUp;
};

internal input_queue
InitializeInputQueue (u8* Memory, s32 MemorySize)
{
    input_queue Result = {};
    s32 EntriesCount = MemorySize / sizeof(input_entry);
    Result.QueueSize = EntriesCount;
    Result.QueueUsed = 0;
    Result.Entries = (input_entry*)Memory;
    return Result;
}

internal void
ResetInputQueue (input_queue* Queue)
{
    Queue->QueueUsed = 0;
}

internal void
AddInputEventEntry (input_queue* Queue, key_code Key, 
                    b32 WasDown, b32 IsDown, b32 ShiftDown, b32 AltDown, b32 CtrlDown, b32 SysDown)
{
    Assert(Queue->QueueUsed < Queue->QueueSize);
    
    input_entry* Entry = Queue->Entries + Queue->QueueUsed++;
    Entry->Key = Key;
    
    Entry->State = (key_state_flags)0;
    if (WasDown) { Entry->State |= KeyState_WasDown; }
    if (IsDown) { Entry->State |= KeyState_IsDown; }
    
    Entry->Modifiers = 0;
    if (ShiftDown) { Entry->Modifiers |= Modifier_Shift; }
    if (CtrlDown) { Entry->Modifiers |= Modifier_Ctrl; }
    if (AltDown) { Entry->Modifiers |= Modifier_Alt; }
    if (SysDown) { Entry->Modifiers |= Modifier_Sys; }
}

internal b32
KeyTransitionedDown (input_entry Entry)
{
    return (!KeyWasDown(Entry) && KeyIsDown(Entry));
}

internal b32
KeyTransitionedUp (input_entry Entry)
{
    return (KeyWasDown(Entry) && !KeyIsDown(Entry));
}

internal b32
KeyHeldDown (input_entry Entry)
{
    return (KeyWasDown(Entry) && KeyIsDown(Entry));
}
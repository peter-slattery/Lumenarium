//
// File: foldhaus_panel.h
// Author: Peter Slattery
// Creation Date: 2019-12-26
//
// Usage:
// Include this file in ONE file in your project.
// Define SetPanelDefinitionExternal
//
#ifndef FOLDHAUS_PANEL_H

enum panel_split_direction
{
    PanelSplit_NoSplit,
    PanelSplit_Horizontal,
    PanelSplit_Vertical,
    
    PanelSplit_Count,
};

typedef struct panel_entry panel_entry;
typedef struct panel panel;

#define PANEL_MODAL_OVERRIDE_CALLBACK(name) void name(panel* ReturningFrom, app_state* State, context Context)
typedef PANEL_MODAL_OVERRIDE_CALLBACK(panel_modal_override_callback);

struct panel
{
    s32 TypeIndex;
    gs_data StateMemory;
    
    panel_entry* ModalOverride;
    panel* IsModalOverrideFor; // TODO(pjs): I don't like that this is panel* but ModalOverride is panel_entry*
    panel_modal_override_callback* ModalOverrideCB;
    
    panel_split_direction SplitDirection;
    r32 SplitPercent;
    
    // TODO(Peter): This REALLY doesn't want to live here
    // Probably belongs in a more generalized PanelInterfaceState or something
    b32 PanelSelectionMenuOpen;
    
    union{
        panel_entry* Left;
        panel_entry* Top;
    };
    union{
        panel_entry* Right;
        panel_entry* Bottom;
    };
};

struct free_panel
{
    panel_entry* Next;
};

struct panel_entry
{
    panel Panel;
    free_panel Free;
};

#define PANEL_INIT_PROC(name) void name(panel* Panel, app_state* State, context Context)
typedef PANEL_INIT_PROC(panel_init_proc);

#define PANEL_CLEANUP_PROC(name) void name(panel* Panel, app_state* State)
typedef PANEL_CLEANUP_PROC(panel_cleanup_proc);

#define PANEL_RENDER_PROC(name) void name(panel* Panel, rect2 PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
typedef PANEL_RENDER_PROC(panel_render_proc);

// NOTE(Peter): This is used by the meta system to generate panel type info
struct panel_definition
{
    char* PanelName;
    s32 PanelNameLength;
    panel_init_proc* Init;
    panel_cleanup_proc* Cleanup;
    panel_render_proc* Render;
    input_command* InputCommands;
    s32 InputCommandsCount;
};

#define PANELS_MAX 16
struct panel_system
{
    panel_definition* PanelDefs;
    u32 PanelDefsCount;
    
    panel_entry Panels[PANELS_MAX];
    u32 PanelsUsed;
    
    panel_entry FreeList;
};

// NOTE(Peter): This representation is used to let external code render and interact
// with panels. It shouldn't be stored across frame boundaries  as the pointers to
// Panel's are liable to change.
struct panel_with_layout
{
    panel* Panel;
    rect2 Bounds;
};

struct panel_layout
{
    panel_with_layout* Panels;
    u32 PanelsCount;
    u32 PanelsMax;
};

/////////////////////////////////
//
//   Book-Keeping
//
/////////////////////////////////

internal void
InitializePanelSystem(panel_system* PanelSystem, panel_definition* PanelDefs, u32 PanelDefsCount)
{
    PanelSystem->FreeList.Free.Next = &PanelSystem->FreeList;
    PanelSystem->PanelDefs = PanelDefs;
    PanelSystem->PanelDefsCount = PanelDefsCount;
}

internal panel_entry*
TakeNewPanelEntry(panel_system* PanelSystem)
{
    panel_entry* FreeEntry = 0;
    if (PanelSystem->FreeList.Free.Next != &PanelSystem->FreeList)
    {
        FreeEntry = PanelSystem->FreeList.Free.Next;
        PanelSystem->FreeList.Free.Next = FreeEntry->Free.Next;
    }
    else
    {
        Assert(PanelSystem->PanelsUsed < PANELS_MAX);
        FreeEntry = PanelSystem->Panels + PanelSystem->PanelsUsed++;
    }
    return FreeEntry;
}

internal void
FreePanelEntry(panel_entry* Entry, panel_system* PanelSystem)
{
    Assert(Entry >= PanelSystem->Panels && Entry <= PanelSystem->Panels + PANELS_MAX);
    Entry->Panel = {0};
    Entry->Free.Next = PanelSystem->FreeList.Free.Next;
    PanelSystem->FreeList.Free.Next = Entry;
}

internal void
FreePanelEntryRecursive(panel_entry* Entry, panel_system* PanelSystem)
{
    if (Entry->Panel.SplitDirection != PanelSplit_NoSplit)
    {
        FreePanelEntryRecursive(Entry->Panel.Left, PanelSystem);
        FreePanelEntryRecursive(Entry->Panel.Right, PanelSystem);
    }
    FreePanelEntry(Entry, PanelSystem);
}

internal void
FreePanelAtIndex(s32 Index, panel_system* PanelSystem)
{
    Assert(Index > 0 && Index < (s32)PanelSystem->PanelsUsed);
    panel_entry* EntryToFree = PanelSystem->Panels + Index;
    EntryToFree->Free.Next = PanelSystem->FreeList.Free.Next;
    PanelSystem->FreeList.Free.Next = EntryToFree;
}

internal panel_entry*
Panel_GetModalOverride(panel_entry* PanelEntry)
{
    panel_entry* Result = PanelEntry;
    if (PanelEntry->Panel.ModalOverride != 0)
    {
        Result = Panel_GetModalOverride(PanelEntry->Panel.ModalOverride);
    }
    return Result;
}

internal panel*
Panel_GetModalOverride(panel* Panel)
{
    panel* Result = Panel;
    if (Panel->ModalOverride != 0)
    {
        Result = &Panel_GetModalOverride(Panel->ModalOverride)->Panel;
    }
    return Result;
}

internal void
Panel_PushModalOverride(panel* Root, panel_entry* Override, panel_modal_override_callback* Callback)
{
    Root->ModalOverride = Override;
    Root->ModalOverrideCB = Callback;
    Override->Panel.IsModalOverrideFor = Root;
}

internal void
Panel_PopModalOverride(panel* Parent, panel_system* System)
{
    // TODO(pjs): Free the overrided panel
    FreePanelEntry(Parent->ModalOverride, System);
    Parent->ModalOverride = 0;
}

internal void
Panel_SetCurrentType(panel* Panel, panel_system* System, s32 NewPanelType, gs_data TypeStateMemory, app_state* State, context Context)
{
    s32 OldTypeIndex = Panel->TypeIndex;
    
    Panel->TypeIndex = NewPanelType;
    Panel->StateMemory = TypeStateMemory;
    
    if(OldTypeIndex >= 0)
    {
        System->PanelDefs[OldTypeIndex].Cleanup(Panel, State);
    }
}

internal void
SetAndInitPanelType(panel* Panel, panel_system* System, s32 NewPanelTypeIndex, app_state* State, context Context)
{
    gs_data EmptyStateData = {0};
    Panel_SetCurrentType(Panel, System, NewPanelTypeIndex, EmptyStateData, State, Context);
    System->PanelDefs[NewPanelTypeIndex].Init(Panel, State, Context);
}

#define Panel_GetStateStruct(p, type) (type*)Panel_GetStateMemory((p), sizeof(type)).Memory
internal gs_data
Panel_GetStateMemory(panel* Panel, u64 Size)
{
    Assert(Panel->StateMemory.Size == Size);
    gs_data Result = Panel->StateMemory;
    return Result;
}

internal panel_entry*
PanelSystem_PushPanel(panel_system* PanelSystem, s32 PanelTypeIndex, app_state* State, context Context)
{
    panel_entry* PanelEntry = TakeNewPanelEntry(PanelSystem);
    SetAndInitPanelType(&PanelEntry->Panel, PanelSystem, PanelTypeIndex, State, Context);
    return PanelEntry;
}

internal void
SplitPanel(panel* Parent, r32 Percent, panel_split_direction SplitDirection, panel_system* PanelSystem, app_state* State, context Context)
{
    if (Percent >= 0.0f && Percent <= 1.0f)
    {
        Parent->SplitDirection = SplitDirection;
        Parent->SplitPercent = Percent;
        
        s32 ParentTypeIndex = Parent->TypeIndex;
        gs_data ParentStateMemory = Parent->StateMemory;
        Parent->Left = TakeNewPanelEntry(PanelSystem);
        Panel_SetCurrentType(&Parent->Left->Panel, PanelSystem, ParentTypeIndex, ParentStateMemory, State, Context);
        
        Parent->Right = TakeNewPanelEntry(PanelSystem);
        Panel_SetCurrentType(&Parent->Right->Panel, PanelSystem, ParentTypeIndex, ParentStateMemory, State, Context);
    }
}

internal void
SplitPanelVertically(panel* Parent, r32 Percent, panel_system* PanelSystem, app_state* State, context Context)
{
    SplitPanel(Parent, Percent, PanelSplit_Vertical, PanelSystem, State, Context);
}

internal void
SplitPanelHorizontally(panel* Parent, r32 Percent, panel_system* PanelSystem, app_state* State, context Context)
{
    SplitPanel(Parent, Percent, PanelSplit_Horizontal, PanelSystem, State, Context);
}

internal void
ConsolidatePanelsKeepOne(panel* Parent, panel_entry* PanelEntryToKeep, panel_system* PanelSystem)
{
    panel_entry* LeftChild = Parent->Left;
    panel_entry* RightChild = Parent->Right;
    
    panel_entry* PanelEntryToDestroy = PanelEntryToKeep == LeftChild ? RightChild : LeftChild;
    
    *Parent = PanelEntryToKeep->Panel;
    
    FreePanelEntry(PanelEntryToKeep, PanelSystem);
    FreePanelEntryRecursive(PanelEntryToDestroy, PanelSystem);
}

/////////////////////////////////
//
//   Rendering And Interaction
//
/////////////////////////////////

internal rect2
GetTopPanelBounds(panel* Panel, rect2 PanelBounds)
{
    rect2 Result = {};
    Result.Min = v2{
        PanelBounds.Min.x,
        LerpR32(Panel->SplitPercent, PanelBounds.Min.y, PanelBounds.Max.y)
    };
    Result.Max = PanelBounds.Max;
    return Result;
}

internal rect2
GetBottomPanelBounds(panel* Panel, rect2 PanelBounds)
{
    rect2 Result = {};
    Result.Min = PanelBounds.Min;
    Result.Max = v2{
        PanelBounds.Max.x,
        LerpR32(Panel->SplitPercent, PanelBounds.Min.y, PanelBounds.Max.y)
    };
    return Result;
}

internal rect2
GetRightPanelBounds(panel* Panel, rect2 PanelBounds)
{
    rect2 Result = {};
    Result.Min = v2{
        LerpR32(Panel->SplitPercent, PanelBounds.Min.x, PanelBounds.Max.x),
        PanelBounds.Min.y
    };
    Result.Max = PanelBounds.Max;
    return Result;
}

internal rect2
GetLeftPanelBounds(panel* Panel, rect2 PanelBounds)
{
    rect2 Result = {};
    Result.Min = PanelBounds.Min;
    Result.Max = v2{
        LerpR32(Panel->SplitPercent, PanelBounds.Min.x, PanelBounds.Max.x),
        PanelBounds.Max.y
    };
    return Result;
}

internal void
LayoutPanel(panel* Panel, rect2 PanelBounds, panel_layout* Layout)
{
    if (Panel->SplitDirection == PanelSplit_NoSplit)
    {
        panel_with_layout* WithLayout = Layout->Panels + Layout->PanelsCount++;
        WithLayout->Panel = Panel_GetModalOverride(Panel);
        WithLayout->Bounds = PanelBounds;
    }
    else if (Panel->SplitDirection == PanelSplit_Horizontal)
    {
        rect2 TopPanelBounds = GetTopPanelBounds(Panel, PanelBounds);
        rect2 BottomPanelBounds = GetBottomPanelBounds(Panel, PanelBounds);
        
        panel* TopPanel = Panel_GetModalOverride(&Panel->Top->Panel);
        panel* BottomPanel = Panel_GetModalOverride(&Panel->Bottom->Panel);
        
        LayoutPanel(&Panel->Top->Panel, TopPanelBounds, Layout);
        LayoutPanel(&Panel->Bottom->Panel, BottomPanelBounds, Layout);
    }
    else if (Panel->SplitDirection == PanelSplit_Vertical)
    {
        rect2 LeftPanelBounds = GetLeftPanelBounds(Panel, PanelBounds);
        rect2 RightPanelBounds = GetRightPanelBounds(Panel, PanelBounds);
        
        panel* LeftPanel = Panel_GetModalOverride(&Panel->Top->Panel);
        panel* RightPanel = Panel_GetModalOverride(&Panel->Bottom->Panel);
        
        LayoutPanel(&Panel->Left->Panel, LeftPanelBounds, Layout);
        LayoutPanel(&Panel->Right->Panel, RightPanelBounds, Layout);
    }
}

internal panel_layout
GetPanelLayout(panel_system* System, rect2 WindowBounds, gs_memory_arena* Storage)
{
    panel_layout Result = {};
    Result.PanelsMax = System->PanelsUsed;
    Result.Panels = PushArray(Storage, panel_with_layout, Result.PanelsMax);
    
    LayoutPanel(&System->Panels[0].Panel, WindowBounds, &Result);
    
    return Result;
}

internal panel_with_layout
GetPanelContainingPoint(v2 Point, panel* Panel, rect2 PanelBounds)
{
    panel_with_layout Result = {0};
    
    if (Panel->SplitDirection == PanelSplit_NoSplit)
    {
        Result.Panel = Panel;
        Result.Bounds = PanelBounds;
    }
    else if (Panel->SplitDirection == PanelSplit_Horizontal)
    {
        rect2 TopPanelBounds = GetTopPanelBounds(Panel, PanelBounds);
        rect2 BottomPanelBounds = GetBottomPanelBounds(Panel, PanelBounds);
        
        if (PointIsInRect(TopPanelBounds, Point))
        {
            Result = GetPanelContainingPoint(Point, &Panel->Top->Panel, TopPanelBounds);
        }
        else if (PointIsInRect(BottomPanelBounds, Point))
        {
            Result = GetPanelContainingPoint(Point, &Panel->Bottom->Panel, BottomPanelBounds);
        }
    }
    else if (Panel->SplitDirection == PanelSplit_Vertical)
    {
        rect2 LeftPanelBounds = GetLeftPanelBounds(Panel, PanelBounds);
        rect2 RightPanelBounds = GetRightPanelBounds(Panel, PanelBounds);
        
        if (PointIsInRect(LeftPanelBounds, Point))
        {
            Result = GetPanelContainingPoint(Point, &Panel->Left->Panel, LeftPanelBounds);
        }
        else if (PointIsInRect(RightPanelBounds, Point))
        {
            Result = GetPanelContainingPoint(Point, &Panel->Right->Panel, RightPanelBounds);
        }
    }
    
    return Result;
}

internal panel_with_layout
GetPanelContainingPoint(v2 Point, panel_system* PanelSystem, rect2 WindowBounds)
{
    panel_with_layout Result = {0};
    if (PanelSystem->PanelsUsed > 0)
    {
        Result = GetPanelContainingPoint(Point, &PanelSystem->Panels[0].Panel, WindowBounds);
    }
    return Result;
}


#define FOLDHAUS_PANEL_H
#endif // FOLDHAUS_PANEL_H
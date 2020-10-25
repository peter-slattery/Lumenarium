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

typedef struct panel panel;

#define PANEL_MODAL_OVERRIDE_CALLBACK(name) void name(panel* ReturningFrom, app_state* State, context Context)
typedef PANEL_MODAL_OVERRIDE_CALLBACK(panel_modal_override_callback);

struct panel
{
    s32 TypeIndex;
    gs_data StateMemory;
    
    panel* ModalOverride;
    panel* IsModalOverrideFor;
    panel_modal_override_callback* ModalOverrideCB;
    
    rect2 Bounds;
    panel_split_direction SplitDirection;
    r32 SplitPercent;
    
    // TODO(Peter): This REALLY doesn't want to live here
    // Probably belongs in a more generalized PanelInterfaceState or something
    b32 PanelSelectionMenuOpen;
    
    panel* Parent;
    
    union{
        panel* Left;
        panel* Top;
    };
    union{
        panel* Right;
        panel* Bottom;
    };
};

struct free_panel
{
    free_panel* Next;
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
    
    panel Panels[PANELS_MAX];
    u32 PanelsUsed;
    
    free_panel* FreeList;
};

/////////////////////////////////
//
//   Book-Keeping
//
/////////////////////////////////

internal void
InitializePanelSystem(panel_system* PanelSystem, panel_definition* PanelDefs, u32 PanelDefsCount)
{
    PanelSystem->FreeList = 0;
    PanelSystem->PanelDefs = PanelDefs;
    PanelSystem->PanelDefsCount = PanelDefsCount;
}

internal panel*
TakeNewPanelEntry(panel_system* PanelSystem)
{
    panel* FreeEntry = 0;
    if (PanelSystem->FreeList != 0)
    {
        free_panel* FreePanel = PanelSystem->FreeList;
        PanelSystem->FreeList = FreePanel->Next;
        FreeEntry = (panel*)PanelSystem->FreeList;
    }
    else
    {
        Assert(PanelSystem->PanelsUsed < PANELS_MAX);
        FreeEntry = PanelSystem->Panels + PanelSystem->PanelsUsed++;
    }
    return FreeEntry;
}

internal void
FreePanelEntry(panel* Panel, panel_system* PanelSystem)
{
    Assert(Panel >= PanelSystem->Panels && Panel <= PanelSystem->Panels + PANELS_MAX);
    
    free_panel* FreeEntry = (free_panel*)Panel;
    FreeEntry->Next = PanelSystem->FreeList;
    PanelSystem->FreeList = FreeEntry;
}

internal void
FreePanelEntryRecursive(panel* Panel, panel_system* PanelSystem)
{
    if (Panel->SplitDirection != PanelSplit_NoSplit)
    {
        FreePanelEntryRecursive(Panel->Left, PanelSystem);
        FreePanelEntryRecursive(Panel->Right, PanelSystem);
    }
    FreePanelEntry(Panel, PanelSystem);
}

internal panel*
Panel_GetModalOverride(panel* Panel)
{
    panel* Result = Panel;
    if (Panel->ModalOverride != 0)
    {
        Result = Panel_GetModalOverride(Panel->ModalOverride);
    }
    return Result;
}

internal void
Panel_PushModalOverride(panel* Root, panel* Override, panel_modal_override_callback* Callback)
{
    Root->ModalOverride = Override;
    Root->ModalOverrideCB = Callback;
    Override->IsModalOverrideFor = Root;
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

internal panel*
PanelSystem_PushPanel(panel_system* PanelSystem, s32 PanelTypeIndex, app_state* State, context Context)
{
    panel* Panel = TakeNewPanelEntry(PanelSystem);
    SetAndInitPanelType(Panel, PanelSystem, PanelTypeIndex, State, Context);
    return Panel;
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
        Panel_SetCurrentType(Parent->Left, PanelSystem, ParentTypeIndex, ParentStateMemory, State, Context);
        Parent->Left->Parent = Parent;
        
        Parent->Right = TakeNewPanelEntry(PanelSystem);
        Panel_SetCurrentType(Parent->Right, PanelSystem, ParentTypeIndex, ParentStateMemory, State, Context);
        Parent->Right->Parent = Parent;
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
ConsolidatePanelsKeepOne(panel* Parent, panel* PanelToKeep, panel_system* PanelSystem)
{
    panel* LeftChild = Parent->Left;
    panel* RightChild = Parent->Right;
    
    panel* PanelToDestroy = PanelToKeep == LeftChild ? RightChild : LeftChild;
    
    *Parent = *PanelToKeep;
    
    FreePanelEntry(PanelToKeep, PanelSystem);
    FreePanelEntryRecursive(PanelToDestroy, PanelSystem);
}

/////////////////////////////////
//
//   Rendering And Interaction
//
/////////////////////////////////

internal rect2
GetTopPanelBounds(panel* Panel)
{
    rect2 Result = {};
    Result.Min = v2{
        Panel->Bounds.Min.x,
        LerpR32(Panel->SplitPercent, Panel->Bounds.Min.y, Panel->Bounds.Max.y)
    };
    Result.Max = Panel->Bounds.Max;
    return Result;
}

internal rect2
GetBottomPanelBounds(panel* Panel)
{
    rect2 Result = {};
    Result.Min = Panel->Bounds.Min;
    Result.Max = v2{
        Panel->Bounds.Max.x,
        LerpR32(Panel->SplitPercent, Panel->Bounds.Min.y, Panel->Bounds.Max.y)
    };
    return Result;
}

internal rect2
GetRightPanelBounds(panel* Panel)
{
    rect2 Result = {};
    Result.Min = v2{
        LerpR32(Panel->SplitPercent, Panel->Bounds.Min.x, Panel->Bounds.Max.x),
        Panel->Bounds.Min.y
    };
    Result.Max = Panel->Bounds.Max;
    return Result;
}

internal rect2
GetLeftPanelBounds(panel* Panel)
{
    rect2 Result = {};
    Result.Min = Panel->Bounds.Min;
    Result.Max = v2{
        LerpR32(Panel->SplitPercent, Panel->Bounds.Min.x, Panel->Bounds.Max.x),
        Panel->Bounds.Max.y
    };
    return Result;
}

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
Panel_UpdateLayout(panel* Panel, rect2 Bounds)
{
    Panel->Bounds = Bounds;
    
    if (Panel->SplitDirection != PanelSplit_NoSplit)
    {
        rect2 LeftOrTopBounds = {};
        rect2 RightOrBottomBounds = {};
        switch (Panel->SplitDirection)
        {
            case PanelSplit_Horizontal:
            {
                LeftOrTopBounds = GetTopPanelBounds(Panel);
                RightOrBottomBounds = GetBottomPanelBounds(Panel);
            } break;
            
            case PanelSplit_Vertical:
            {
                LeftOrTopBounds = GetLeftPanelBounds(Panel);
                RightOrBottomBounds = GetRightPanelBounds(Panel);
            } break;
            
            InvalidDefaultCase;
        }
        
        Panel_UpdateLayout(Panel->Left, LeftOrTopBounds);
        Panel_UpdateLayout(Panel->Right, RightOrBottomBounds);
    }
}

internal void
PanelSystem_UpdateLayout(panel_system* System, rect2 WindowBounds)
{
    panel* Root = System->Panels;
    Panel_UpdateLayout(Root, WindowBounds);
}

internal panel*
GetPanelContainingPoint(v2 Point, panel* Panel)
{
    panel* Result = 0;
    
    if (PointIsInRect(Panel->Bounds, Point))
    {
        switch (Panel->SplitDirection)
        {
            case PanelSplit_NoSplit:
            {
                Result = Panel;
            }break;
            
            case PanelSplit_Vertical:
            case PanelSplit_Horizontal:
            {
                if (PointIsInRect(Panel->Left->Bounds, Point))
                {
                    Result = GetPanelContainingPoint(Point, Panel->Left);
                }
                else if (PointIsInRect(Panel->Right->Bounds, Point))
                {
                    Result = GetPanelContainingPoint(Point, Panel->Right);
                }
            }break;
            
            InvalidDefaultCase;
        }
    }
    
    return Result;
}


#define FOLDHAUS_PANEL_H
#endif // FOLDHAUS_PANEL_H
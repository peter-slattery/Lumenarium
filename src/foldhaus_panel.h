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

typedef struct panel panel;

enum panel_split_direction
{
    PanelSplit_NoSplit,
    PanelSplit_Horizontal,
    PanelSplit_Vertical,
    
    PanelSplit_Count,
};

typedef struct panel_entry panel_entry;

struct panel
{
    s32 PanelDefinitionIndex;
    
    panel_split_direction SplitDirection;
    r32 SplitPercent;
    
    // TODO(Peter): This REALLY doesn't want to live here
    // Probably belongs in a more generalized PanelInterfaceState or something
    b32 PanelSelectionMenuOpen;
    
    u8* PanelStateMemory;
    u32 PanelStateMemorySize;
    
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

#define PANELS_MAX 16
struct panel_system
{
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
    rect Bounds;
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
InitializePanelSystem(panel_system* PanelSystem)
{
    PanelSystem->FreeList.Free.Next = &PanelSystem->FreeList;
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

internal panel*
TakeNewPanel(panel_system* PanelSystem)
{
    panel* Result = 0;
    panel_entry* FreeEntry = TakeNewPanelEntry(PanelSystem);
    Result = &FreeEntry->Panel;
    
    *Result = {0};
    Result->PanelDefinitionIndex = -1;
    
    return Result;
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

internal void
SplitPanelVertically(panel* Parent, r32 Percent, rect ParentBounds, panel_system* PanelSystem)
{
    r32 SplitX = GSLerp(ParentBounds.Min.x, ParentBounds.Max.x, Percent);
    if (SplitX > ParentBounds.Min.x && SplitX < ParentBounds.Max.x)
    {
        Parent->SplitDirection = PanelSplit_Vertical;
        Parent->SplitPercent = Percent;
        
        Parent->Left = TakeNewPanelEntry(PanelSystem);
        Parent->Left->Panel.PanelDefinitionIndex = Parent->PanelDefinitionIndex;
        
        Parent->Right = TakeNewPanelEntry(PanelSystem);
        Parent->Right->Panel.PanelDefinitionIndex = Parent->PanelDefinitionIndex;
    }
}

internal void
SplitPanelHorizontally(panel* Parent, r32 Percent, rect ParentBounds, panel_system* PanelSystem)
{
    r32 SplitY = GSLerp(ParentBounds.Min.y, ParentBounds.Max.y, Percent);
    if (SplitY > ParentBounds.Min.y && SplitY < ParentBounds.Max.y)
    {
        Parent->SplitDirection = PanelSplit_Horizontal;
        Parent->SplitPercent = Percent;
        
        Parent->Bottom = TakeNewPanelEntry(PanelSystem);
        Parent->Bottom->Panel.PanelDefinitionIndex = Parent->PanelDefinitionIndex;
        
        Parent->Top = TakeNewPanelEntry(PanelSystem);
        Parent->Top->Panel.PanelDefinitionIndex = Parent->PanelDefinitionIndex;
    }
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

internal rect
GetTopPanelBounds(panel* Panel, rect PanelBounds)
{
    rect Result = {};
    Result.Min = v2{
        PanelBounds.Min.x,
        GSLerp(PanelBounds.Min.y, PanelBounds.Max.y, Panel->SplitPercent)
    };
    Result.Max = PanelBounds.Max;
    return Result;
}

internal rect
GetBottomPanelBounds(panel* Panel, rect PanelBounds)
{
    rect Result = {};
    Result.Min = PanelBounds.Min;
    Result.Max = v2{
        PanelBounds.Max.x,
        GSLerp(PanelBounds.Min.y, PanelBounds.Max.y, Panel->SplitPercent)
    };
    return Result;
}

internal rect
GetRightPanelBounds(panel* Panel, rect PanelBounds)
{
    rect Result = {};
    Result.Min = v2{
        GSLerp(PanelBounds.Min.x, PanelBounds.Max.x, Panel->SplitPercent),
        PanelBounds.Min.y
    };
    Result.Max = PanelBounds.Max;
    return Result;
}

internal rect
GetLeftPanelBounds(panel* Panel, rect PanelBounds)
{
    rect Result = {};
    Result.Min = PanelBounds.Min;
    Result.Max = v2{
        GSLerp(PanelBounds.Min.x, PanelBounds.Max.x, Panel->SplitPercent),
        PanelBounds.Max.y
    };
    return Result;
}

internal void
LayoutPanel(panel* Panel, rect PanelBounds, panel_layout* Layout)
{
    if (Panel->SplitDirection == PanelSplit_NoSplit)
    {
        panel_with_layout* WithLayout = Layout->Panels + Layout->PanelsCount++;
        WithLayout->Panel = Panel;
        WithLayout->Bounds = PanelBounds;
    }
    else if (Panel->SplitDirection == PanelSplit_Horizontal)
    {
        rect TopPanelBounds = GetTopPanelBounds(Panel, PanelBounds);
        rect BottomPanelBounds = GetBottomPanelBounds(Panel, PanelBounds);
        LayoutPanel(&Panel->Top->Panel, TopPanelBounds, Layout);
        LayoutPanel(&Panel->Bottom->Panel, BottomPanelBounds, Layout);
    }
    else if (Panel->SplitDirection == PanelSplit_Vertical)
    {
        rect LeftPanelBounds = GetLeftPanelBounds(Panel, PanelBounds);
        rect RightPanelBounds = GetRightPanelBounds(Panel, PanelBounds);
        LayoutPanel(&Panel->Left->Panel, LeftPanelBounds, Layout);
        LayoutPanel(&Panel->Right->Panel, RightPanelBounds, Layout);
    }
}

internal panel_layout
GetPanelLayout(panel_system* System, rect WindowBounds, memory_arena* Storage)
{
    panel_layout Result = {};
    Result.PanelsMax = System->PanelsUsed;
    Result.Panels = PushArray(Storage, panel_with_layout, Result.PanelsMax);
    
    LayoutPanel(&System->Panels[0].Panel, WindowBounds, &Result);
    
    return Result;
}

struct panel_and_bounds
{
    panel* Panel;
    rect Bounds;
};

internal panel_and_bounds
GetPanelContainingPoint(v2 Point, panel* Panel, rect PanelBounds)
{
    panel_and_bounds Result = {0};
    
    if (Panel->SplitDirection == PanelSplit_NoSplit)
    {
        Result.Panel = Panel;
        Result.Bounds = PanelBounds;
    }
    else if (Panel->SplitDirection == PanelSplit_Horizontal)
    {
        rect TopPanelBounds = GetTopPanelBounds(Panel, PanelBounds);
        rect BottomPanelBounds = GetBottomPanelBounds(Panel, PanelBounds);
        
        if (PointIsInRange(Point, TopPanelBounds.Min, TopPanelBounds.Max))
        {
            Result = GetPanelContainingPoint(Point, &Panel->Top->Panel, TopPanelBounds);
        }
        else if (PointIsInRange(Point, BottomPanelBounds.Min, BottomPanelBounds.Max))
        {
            Result = GetPanelContainingPoint(Point, &Panel->Bottom->Panel, BottomPanelBounds);
        }
    }
    else if (Panel->SplitDirection == PanelSplit_Vertical)
    {
        rect LeftPanelBounds = GetLeftPanelBounds(Panel, PanelBounds);
        rect RightPanelBounds = GetRightPanelBounds(Panel, PanelBounds);
        
        if (PointIsInRange(Point, LeftPanelBounds.Min, LeftPanelBounds.Max))
        {
            Result = GetPanelContainingPoint(Point, &Panel->Left->Panel, LeftPanelBounds);
        }
        else if (PointIsInRange(Point, RightPanelBounds.Min, RightPanelBounds.Max))
        {
            Result = GetPanelContainingPoint(Point, &Panel->Right->Panel, RightPanelBounds);
        }
    }
    
    return Result;
}

internal panel_and_bounds
GetPanelContainingPoint(v2 Point, panel_system* PanelSystem, rect WindowBounds)
{
    panel_and_bounds Result = {0};
    if (PanelSystem->PanelsUsed > 0)
    {
        Result = GetPanelContainingPoint(Point, &PanelSystem->Panels[0].Panel, WindowBounds);
    }
    return Result;
}


#define FOLDHAUS_PANEL_H
#endif // FOLDHAUS_PANEL_H
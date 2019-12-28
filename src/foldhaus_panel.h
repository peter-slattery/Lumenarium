/*
File: foldhaus_panel.cpp
Description: a system for laying out panels on a screen
Author: Peter Slattery
Creation Date: 2019-12-26

Usage:
Include this file in ONE file in your project. 
Define both RenderPanel and SetPanelDefinitionExternal

*/

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
struct panel_layout
{
    panel_entry Panels[PANELS_MAX];
    u32 PanelsUsed;
    
    panel_entry FreeList;
};

internal void SetPanelDefinitionExternal(panel* Panel, s32 OldPanelDefinitionIndex, s32 NewPanelDefinitionIndex);
internal void RenderPanel(panel* Panel, v2 PanelMin, v2 PanelMax, v2 WindowMin, v2 WindowMax, render_command_buffer* RenderBuffer, app_state* State, context Context, mouse_state Mouse);


/////////////////////////////////
//
//   Book-Keeping
//
/////////////////////////////////

internal void
InitializePanelLayout(panel_layout* Layout)
{
    Layout->FreeList.Free.Next = &Layout->FreeList;
}

internal panel_entry*
TakeNewPanelEntry(panel_layout* Layout)
{
    panel_entry* FreeEntry = 0;
    if (Layout->FreeList.Free.Next != &Layout->FreeList)
    {
        FreeEntry = Layout->FreeList.Free.Next;
        Layout->FreeList.Free.Next = FreeEntry->Free.Next;
    }
    else
    {
        Assert(Layout->PanelsUsed < PANELS_MAX);
        FreeEntry = Layout->Panels + Layout->PanelsUsed++;
    }
    return FreeEntry;
}

internal panel*
TakeNewPanel(panel_layout* Layout)
{
    panel* Result = 0;
    panel_entry* FreeEntry = TakeNewPanelEntry(Layout);
    Result = &FreeEntry->Panel;
    
    *Result = {0};
    Result->PanelDefinitionIndex = -1;
    
    return Result;
}

internal void
FreePanelEntry(panel_entry* Entry, panel_layout* Layout)
{
    Assert(Entry >= Layout->Panels && Entry <= Layout->Panels + PANELS_MAX);
    Entry->Panel = {0};
    Entry->Free.Next = Layout->FreeList.Free.Next;
    Layout->FreeList.Free.Next = Entry;
}

internal void
FreePanelAtIndex(s32 Index, panel_layout* Layout)
{
    Assert(Index > 0 && Index < (s32)Layout->PanelsUsed);
    panel_entry* EntryToFree = Layout->Panels + Index;
    EntryToFree->Free.Next = Layout->FreeList.Free.Next;
    Layout->FreeList.Free.Next = EntryToFree;
}

internal void
SplitPanelVertically(panel* Parent, r32 Percent, rect ParentBounds, panel_layout* Layout)
{
    r32 SplitX = GSLerp(ParentBounds.Min.x, ParentBounds.Max.x, Percent);
    if (SplitX > ParentBounds.Min.x && SplitX < ParentBounds.Max.x)
    {
        Parent->SplitDirection = PanelSplit_Vertical;
        Parent->SplitPercent = Percent;
        
        Parent->Left = TakeNewPanelEntry(Layout);
        Parent->Left->Panel.PanelDefinitionIndex = Parent->PanelDefinitionIndex;
        
        Parent->Right = TakeNewPanelEntry(Layout);
        Parent->Right->Panel.PanelDefinitionIndex = Parent->PanelDefinitionIndex;
    }
}

internal void
SplitPanelHorizontally(panel* Parent, r32 Percent, rect ParentBounds, panel_layout* Layout)
{
    r32 SplitY = GSLerp(ParentBounds.Min.y, ParentBounds.Max.y, Percent);
    if (SplitY > ParentBounds.Min.y && SplitY < ParentBounds.Max.y)
    {
        Parent->SplitDirection = PanelSplit_Horizontal;
        Parent->SplitPercent = Percent;
        
        Parent->Bottom = TakeNewPanelEntry(Layout);
        Parent->Bottom->Panel.PanelDefinitionIndex = Parent->PanelDefinitionIndex;
        
        Parent->Top = TakeNewPanelEntry(Layout);
        Parent->Top->Panel.PanelDefinitionIndex = Parent->PanelDefinitionIndex;
    }
}

internal void
ConsolidatePanelsKeepOne(panel* Parent, panel_entry* PanelEntryToKeep, panel_layout* Layout)
{
    panel_entry* LeftChild = Parent->Left;
    panel_entry* RightChild = Parent->Right;
    
    *Parent = PanelEntryToKeep->Panel;
    Parent->SplitDirection = PanelSplit_NoSplit;
    
    FreePanelEntry(LeftChild, Layout);
    FreePanelEntry(RightChild, Layout);
}

internal void
SetPanelDefinition(panel* Panel, s32 NewDefinitionIndex)
{
    s32 OldDefinitionIndex = Panel->PanelDefinitionIndex;
    Panel->PanelDefinitionIndex = NewDefinitionIndex;
    SetPanelDefinitionExternal(Panel, OldDefinitionIndex, NewDefinitionIndex);
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
GetPanelContainingPoint(v2 Point, panel_layout* Layout, rect WindowBounds)
{
    panel_and_bounds Result = {0};
    if (Layout->PanelsUsed > 0)
    {
        Result = GetPanelContainingPoint(Point, &Layout->Panels[0].Panel, WindowBounds);
    }
    return Result;
}

internal void
HandleMousePanelInteractionOrRecurse(panel* Panel, rect PanelBounds, panel_layout* PanelLayout, mouse_state Mouse)
{
    r32 PanelEdgeClickMaxDistance = 4;
    
    // TODO(Peter): Need a way to calculate this button's position more systemically
    if (Panel->SplitDirection == PanelSplit_NoSplit 
        && PointIsInRange(Mouse.DownPos, PanelBounds.Min, PanelBounds.Min + v2{25, 25}))
    {
        r32 XDistance = GSAbs(Mouse.Pos.x - Mouse.DownPos.x);
        r32 YDistance = GSAbs(Mouse.Pos.y - Mouse.DownPos.y);
        
        if (XDistance > YDistance)
        {
            r32 XPercent = (Mouse.Pos.x - PanelBounds.Min.x) / Width(PanelBounds);
            SplitPanelVertically(Panel, XPercent, PanelBounds, PanelLayout);
        }
        else
        {
            r32 YPercent = (Mouse.Pos.y - PanelBounds.Min.y) / Height(PanelBounds);
            SplitPanelHorizontally(Panel, YPercent, PanelBounds, PanelLayout);
        }
    }
    else if (Panel->SplitDirection == PanelSplit_Horizontal)
    {
        r32 SplitY = GSLerp(PanelBounds.Min.y, PanelBounds.Max.y, Panel->SplitPercent);
        r32 ClickDistanceFromSplit = GSAbs(Mouse.DownPos.y - SplitY);
        if (ClickDistanceFromSplit < PanelEdgeClickMaxDistance)
        {
            r32 NewSplitY = Mouse.Pos.y;
            if (NewSplitY <= PanelBounds.Min.y)
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Top, PanelLayout);
            }
            else if (NewSplitY >= PanelBounds.Max.y)
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Bottom, PanelLayout);
            }
            else
            {
                Panel->SplitPercent = (NewSplitY  - PanelBounds.Min.y) / Height(PanelBounds);
            }
        }
        else
        {
            rect TopPanelBounds = GetTopPanelBounds(Panel, PanelBounds);
            rect BottomPanelBounds = GetBottomPanelBounds(Panel, PanelBounds);
            HandleMousePanelInteractionOrRecurse(&Panel->Bottom->Panel, BottomPanelBounds, PanelLayout, Mouse);
            HandleMousePanelInteractionOrRecurse(&Panel->Top->Panel, TopPanelBounds, PanelLayout, Mouse);
        }
    }
    else if (Panel->SplitDirection == PanelSplit_Vertical)
    {
        r32 SplitX = GSLerp(PanelBounds.Min.x, PanelBounds.Max.x, Panel->SplitPercent);
        r32 ClickDistanceFromSplit = GSAbs(Mouse.DownPos.x - SplitX);
        if (ClickDistanceFromSplit < PanelEdgeClickMaxDistance)
        {
            r32 NewSplitX = Mouse.Pos.x;
            if (NewSplitX <= PanelBounds.Min.x)
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Right, PanelLayout);
            }
            else if (NewSplitX >= PanelBounds.Max.x)
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Left, PanelLayout);
            }
            else
            {
                Panel->SplitPercent = (NewSplitX  - PanelBounds.Min.x) / Width(PanelBounds); 
            }
        }
        else
        {
            rect LeftPanelBounds = GetLeftPanelBounds(Panel, PanelBounds);
            rect RightPanelBounds = GetRightPanelBounds(Panel, PanelBounds);
            HandleMousePanelInteractionOrRecurse(&Panel->Left->Panel, LeftPanelBounds, PanelLayout, Mouse);
            HandleMousePanelInteractionOrRecurse(&Panel->Right->Panel, RightPanelBounds, PanelLayout, Mouse);
        }
    }
}

internal void
HandleMousePanelInteraction(panel_layout* PanelLayout, rect WindowBounds, mouse_state Mouse)
{
    r32 PanelEdgeClickMaxDistance = 4;
    
    if (MouseButtonTransitionedUp(Mouse.LeftButtonState))
    {
        Assert(PanelLayout->PanelsUsed > 0);
        panel* FirstPanel = &PanelLayout->Panels[0].Panel;
        HandleMousePanelInteractionOrRecurse(FirstPanel, WindowBounds, PanelLayout, Mouse);
    }
}

internal void
DrawPanelBorder(panel Panel, v2 PanelMin, v2 PanelMax, v4 Color, render_command_buffer* RenderBuffer)
{
    PushRenderBoundingBox2D(RenderBuffer, PanelMin, PanelMax, 1, Color);
}

internal void
DrawPanelOrRecurse(panel* Panel, rect PanelBounds, rect WindowRect, render_command_buffer* RenderBuffer, interface_config Interface, mouse_state Mouse, app_state* State, context Context)
{
    if (Panel->SplitDirection == PanelSplit_NoSplit)
    {
        RenderPanel(Panel, PanelBounds.Min, PanelBounds.Max, WindowRect.Min, WindowRect.Max, RenderBuffer, State, Context, Mouse);
        v4 BorderColor = v4{0, 0, 0, 1};
        
#if 0
        if (PointIsInRange(Mouse.Pos, PanelMin, PanelMax))
        {
            BorderColor = v4{1, 0, 1, 1};
        }
#endif
        
        PushRenderOrthographic(RenderBuffer, WindowRect.Min.x, WindowRect.Min.y, WindowRect.Max.x, WindowRect.Max.y);
        DrawPanelBorder(*Panel, PanelBounds.Min, PanelBounds.Max, BorderColor, RenderBuffer);
    }
    else if (Panel->SplitDirection == PanelSplit_Horizontal)
    {
        rect TopPanelBounds = GetTopPanelBounds(Panel, PanelBounds);
        rect BottomPanelBounds = GetBottomPanelBounds(Panel, PanelBounds);
        
        DrawPanelOrRecurse(&Panel->Bottom->Panel, BottomPanelBounds, WindowRect, RenderBuffer, Interface, Mouse, State, Context);
        DrawPanelOrRecurse(&Panel->Top->Panel, TopPanelBounds, WindowRect, RenderBuffer, Interface, Mouse, State, Context);
    }
    else if (Panel->SplitDirection == PanelSplit_Vertical)
    {
        rect LeftPanelBounds = GetLeftPanelBounds(Panel, PanelBounds);
        rect RightPanelBounds = GetRightPanelBounds(Panel, PanelBounds);
        
        DrawPanelOrRecurse(&Panel->Left->Panel, LeftPanelBounds, WindowRect, RenderBuffer, Interface, Mouse, State, Context);
        DrawPanelOrRecurse(&Panel->Right->Panel, RightPanelBounds, WindowRect, RenderBuffer, Interface, Mouse, State, Context);
    }
}

internal void
DrawAllPanels(panel_layout* PanelLayout, rect WindowBounds, render_command_buffer* RenderBuffer, interface_config Interface, mouse_state Mouse, app_state* State, context Context)
{
    Assert(PanelLayout->PanelsUsed > 0);
    panel* FirstPanel = &PanelLayout->Panels[0].Panel;
    DrawPanelOrRecurse(FirstPanel, WindowBounds, WindowBounds, RenderBuffer, Interface, Mouse, State, Context);
}

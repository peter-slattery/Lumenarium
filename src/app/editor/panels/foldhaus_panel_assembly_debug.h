//
// File: foldhaus_panel_assembly_debug.h
// Author: Peter Slattery
// Creation Date: 2021-01-15
//
#ifndef FOLDHAUS_PANEL_ASSEMBLY_DEBUG_H

GSMetaTag(panel_init);
GSMetaTag(panel_type_file_view);
internal void
AssemblyDebug_Init(panel* Panel, app_state* State, context Context)
{
}

GSMetaTag(panel_cleanup);
GSMetaTag(panel_type_file_view);
internal void
AssemblyDebug_Cleanup(panel* Panel, app_state* State)
{
    
}

// TODO(pjs): This is really blumen specific
#define FSC(f,c) FlowerStripToChannel((f), (c))
internal u8
FlowerStripToChannel(u8 Flower, u8 Channel)
{
    Assert(Flower < 3);
    Assert(Channel < 8);
    
    u8 Result = 0;
    Result |= (Flower & 0x03) << 3;
    Result |= (Channel & 0x07);
    
    return Result;
}

GSMetaTag(panel_render);
GSMetaTag(panel_type_file_view);
internal void
AssemblyDebug_Render(panel* Panel, rect2 PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    ui_interface* Interface = &State->Interface;
    ui_PushLayout(Interface, PanelBounds, LayoutDirection_TopDown, MakeString("Assembly Debug Layout"));
    
    gs_string OverrideStr = MakeString(OverrideTypeStrings[State->AssemblyDebugState.Override]);
    if (ui_BeginLabeledDropdown(Interface, MakeString("Override"), OverrideStr))
    {
        for (u32 i = 0; i < ADS_Override_Count; i++)
        {
            if (ui_Button(Interface, MakeString(OverrideTypeStrings[i])))
            {
                State->AssemblyDebugState.Override = (override_type)i;
            }
        }
    }
    
    if (State->AssemblyDebugState.Override == ADS_Override_TagWhite ||
        State->AssemblyDebugState.Override == ADS_Override_TagStripWhite)
    {
        ui_LabeledTextEntry(Interface, MakeString("Tag Name"), &State->AssemblyDebugState.TagName);
        ui_LabeledTextEntry(Interface, MakeString("Tag Value"), &State->AssemblyDebugState.TagValue);
        
        if (State->AssemblyDebugState.Override == ADS_Override_TagStripWhite)
        {
            ui_EndLabeledDropdown(Interface);
            State->AssemblyDebugState.TargetAssembly = ui_LabeledTextEntryU64(Interface, MakeString("Assembly"), State->AssemblyDebugState.TargetAssembly);
            
            State->AssemblyDebugState.TargetStrip = ui_LabeledTextEntryU64(Interface, MakeString("Strip"), State->AssemblyDebugState.TargetStrip);
        }
    }
    else if (State->AssemblyDebugState.Override == ADS_Override_ChannelWhite)
    {
        u64 Board = 0;
        u64 Strip = 0;
        Board = ui_LabeledTextEntryU64(Interface, MakeString("Board"), Board);
        Strip = ui_LabeledTextEntryU64(Interface, MakeString("Strip"), Strip);
        
        State->AssemblyDebugState.TargetChannel = FSC(Board, Strip);
    }
    else
    {
        ui_EndLabeledDropdown(Interface);
        State->AssemblyDebugState.TargetAssembly = ui_LabeledTextEntryU64(Interface, MakeString("Assembly"), State->AssemblyDebugState.TargetAssembly);
        
        State->AssemblyDebugState.TargetStrip = ui_LabeledTextEntryU64(Interface, MakeString("Strip"), State->AssemblyDebugState.TargetStrip);
    }
}

#define FOLDHAUS_PANEL_ASSEMBLY_DEBUG_H
#endif // FOLDHAUS_PANEL_ASSEMBLY_DEBUG_H
//
// File: foldhaus_panel_file_view.h
// Author: Peter Slattery
// Creation Date: 2020-03-08
//
#ifndef FOLDHAUS_PANEL_FILE_VIEW_H

struct file_view_state
{
    gs_string WorkingDirectory;
    gs_memory_arena FileNamesArena;
    gs_file_info_array FileNames;
};

input_command* FileView_Commands = 0;
s32 FileView_CommandsCount = 0;

internal void
FileViewUpdateWorkingDirectory(gs_const_string WorkingDirectory, file_view_state* State, context Context)
{
    ClearArena(&State->FileNamesArena);
    
    gs_const_string SanitizedDirectory = WorkingDirectory;
    
    u32 LastSlashIndex = FindLast(SanitizedDirectory, '\\');
    gs_const_string LastDir = Substring(SanitizedDirectory, LastSlashIndex + 1, SanitizedDirectory.Length);
    if (StringsEqual(LastDir, ConstString("..")))
    {
        u32 SecondLastSlashIndex = FindLast(SanitizedDirectory, LastSlashIndex - 1, '\\');
        SanitizedDirectory = Substring(SanitizedDirectory, 0, SecondLastSlashIndex);
    }
    
    State->WorkingDirectory = PushString(&State->FileNamesArena, WorkingDirectory.Length + 2);
    PrintF(&State->WorkingDirectory, "%S", SanitizedDirectory);
    if (State->WorkingDirectory.Str[State->WorkingDirectory.Length - 1] != '\\')
    {
        AppendPrintF(&State->WorkingDirectory, "\\");
    }
    if (State->WorkingDirectory.Str[State->WorkingDirectory.Length - 1] != '*')
    {
        AppendPrintF(&State->WorkingDirectory, "*");
    }
    
    State->FileNames = EnumerateDirectory(Context.ThreadContext.FileHandler, &State->FileNamesArena, State->WorkingDirectory.ConstString, EnumerateDirectory_IncludeDirectories);
}

GSMetaTag(panel_init);
GSMetaTag(panel_type_file_view);
internal void
FileView_Init(panel* Panel, app_state* State, context Context)
{
    // TODO: :FreePanelMemory
    file_view_state* FileViewState = PushStruct(&State->Permanent, file_view_state);
    Panel->PanelStateMemory = (u8*)FileViewState;
    FileViewState->FileNamesArena = CreateMemoryArena(Context.ThreadContext.Allocator);
    
#if 0
    FileViewState->WorkingDirectory = MakeString(PushArray(&State->Permanent, char, 256), 256);
    PrintF(&FileViewState->WorkingDirectory, "C:\\");
#endif
    FileViewUpdateWorkingDirectory(ConstString("C:\\projects\\Lumenarium\\src\\app"), FileViewState, Context);
}

GSMetaTag(panel_cleanup);
GSMetaTag(panel_type_file_view);
internal void
FileView_Cleanup(panel* Panel, app_state* State)
{
    
}

GSMetaTag(panel_render);
GSMetaTag(panel_type_file_view);
internal void
FileView_Render(panel Panel, rect2 PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    file_view_state* FileViewState = (file_view_state*)Panel.PanelStateMemory;
    ui_layout Layout = ui_CreateLayout(State->Interface, PanelBounds);
    
    // Header
    rect2 HeaderBounds = ui_ReserveElementBounds(&Layout);
    
    // File Display
    for (u32 i = 0; i < FileViewState->FileNames.Count; i++)
    {
        gs_file_info File = FileViewState->FileNames.Values[i];
        gs_string PathString = PushString(State->Transient, File.Path.Length);
        PrintF(&PathString, "%S", File.Path);
        if (ui_LayoutListButton(&State->Interface, &Layout, PathString, i))
        {
            if (File.IsDirectory)
            {
                FileViewUpdateWorkingDirectory(File.Path, FileViewState, Context);
            }
            else
            {
                // TODO(pjs): Select the file
            }
        }
        //ui_LayoutDrawString(&State->Interface, &Layout, PathString, State->Interface.Style.TextColor);
    }
    
}



#define FOLDHAUS_PANEL_FILE_VIEW_H
#endif // FOLDHAUS_PANEL_FILE_VIEW_H
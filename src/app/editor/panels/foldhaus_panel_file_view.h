//
// File: foldhaus_panel_file_view.h
// Author: Peter Slattery
// Creation Date: 2020-03-08
//
#ifndef FOLDHAUS_PANEL_FILE_VIEW_H

enum file_view_mode
{
    FileViewMode_Load,
    FileViewMode_Save,
};

struct file_view_state
{
    file_view_mode Mode;
    
    gs_string WorkingDirectory;
    gs_memory_arena FileNamesArena;
    gs_file_info_array FileNames;
    
    gs_file_info SelectedFile;
};

internal void
FileView_SetMode(panel* Panel, file_view_mode Mode)
{
    file_view_state* FileViewState = Panel_GetStateStruct(Panel, file_view_state);
    FileViewState->Mode = Mode;
}

internal void
FileView_Exit_(panel* FileViewPanel, app_state* State, context Context)
{
    // TODO(pjs): Free State->FileNamesArena
    
    Assert(FileViewPanel->IsModalOverrideFor != 0);
    panel* ReturnTo = FileViewPanel->IsModalOverrideFor;
    if (ReturnTo->ModalOverrideCB)
    {
        ReturnTo->ModalOverrideCB(FileViewPanel, State, Context);
    }
    Panel_PopModalOverride(ReturnTo, &State->PanelSystem);
}

global input_command* FileView_Commands = 0;
s32 FileView_CommandsCount = 0;

internal void
FileView_UpdateWorkingDirectory(gs_const_string WorkingDirectory, file_view_state* State, context Context)
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
    else if (StringsEqual(LastDir, ConstString(".")) && LastDir.Length > 1)
    {
        SanitizedDirectory = Substring(SanitizedDirectory, 0, LastSlashIndex);
    }
    
    gs_file_info NewWorkingDirectory = GetFileInfo(Context.ThreadContext.FileHandler, SanitizedDirectory);
    if (NewWorkingDirectory.IsDirectory)
    {
        // NOTE(pjs): we might be printing from State->WorkingDirectory to State->WorkingDirectory
        // in some cases. Shouldn't be a problem but it is unnecessary
        PrintF(&State->WorkingDirectory, "%S", WorkingDirectory);
        
        State->FileNames = EnumerateDirectory(Context.ThreadContext.FileHandler, &State->FileNamesArena, State->WorkingDirectory.ConstString, EnumerateDirectory_IncludeDirectories);
    }
}

GSMetaTag(panel_init);
GSMetaTag(panel_type_file_view);
internal void
FileView_Init(panel* Panel, app_state* State, context Context)
{
    // TODO: :FreePanelMemory
    file_view_state* FileViewState = PushStruct(&State->Permanent, file_view_state);
    Panel->StateMemory = StructToData(FileViewState, file_view_state);
    FileViewState->FileNamesArena = CreateMemoryArena(Context.ThreadContext.Allocator);
    
    // TODO(pjs): this shouldn't be stored in permanent
    FileViewState->WorkingDirectory = PushString(&State->Permanent, 256);
    FileView_UpdateWorkingDirectory(ConstString("."), FileViewState, Context);
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
FileView_Render(panel* Panel, rect2 PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    file_view_state* FileViewState = Panel_GetStateStruct(Panel, file_view_state);
    Assert(FileViewState->Mode == FileViewMode_Save);
    
    ui_PushLayout(&State->Interface, PanelBounds, LayoutDirection_TopDown, MakeString("FileView Layout"));
    {
        if (ui_Button(&State->Interface, MakeString("Exit")))
        {
            FileView_Exit_(Panel, State, Context);
        }
        
        // Header
        if (ui_TextEntry(&State->Interface, MakeString("pwd"), &FileViewState->WorkingDirectory))
        {
            // if last character is a slash, update pwd, and clear the filter string
            // otherwise update the filter string
            gs_string Pwd = FileViewState->WorkingDirectory;
            char LastChar = Pwd.Str[Pwd.Length - 1];
            if (LastChar == '\\' || LastChar == '/')
            {
                FileView_UpdateWorkingDirectory(Pwd.ConstString, FileViewState, Context);
            }
            else
            {
                
            }
        }
        
        // File Display
        ui_BeginList(&State->Interface, MakeString("Files"), 10, FileViewState->FileNames.Count);
        for (u32 i = 0; i < FileViewState->FileNames.Count; i++)
        {
            gs_file_info File = FileViewState->FileNames.Values[i];
            
            u32 LastSlashIndex = FindLast(File.Path, '\\');
            gs_const_string FileName = Substring(File.Path, LastSlashIndex + 1, File.Path.Length);
            gs_string PathString = PushString(State->Transient, FileName.Length);
            PrintF(&PathString, "%S", FileName);
            
            if (ui_LayoutListButton(&State->Interface, PathString, i))
            {
                if (File.IsDirectory)
                {
                    FileView_UpdateWorkingDirectory(File.Path, FileViewState, Context);
                }
                else
                {
                    FileViewState->SelectedFile = File;
                    FileView_Exit_(Panel, State, Context);
                }
            }
        }
        ui_EndList(&State->Interface);
    }
    ui_PopLayout(&State->Interface, MakeString("FileView Layout"));
}


#define FOLDHAUS_PANEL_FILE_VIEW_H
#endif // FOLDHAUS_PANEL_FILE_VIEW_H
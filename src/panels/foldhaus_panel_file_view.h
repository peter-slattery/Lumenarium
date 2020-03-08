//
// File: foldhaus_panel_file_view.h
// Author: Peter Slattery
// Creation Date: 2020-03-08
//
#ifndef FOLDHAUS_PANEL_FILE_VIEW_H

struct file_view_state
{
    string WorkingDirectory;
};

input_command* FileView_Commands = 0;
s32 FileView_CommandsCount = 0;

GSMetaTag(panel_init);
GSMetaTag(panel_type_file_view);
internal void
FileView_Init(panel* Panel, app_state* State)
{
    // TODO: :FreePanelMemory
    file_view_state* FileViewState = PushStruct(&State->Permanent, file_view_state);
    FileViewState->WorkingDirectory = MakeString(PushArray(&State->Permanent, char, 256), 256);
    PrintF(&FileViewState->WorkingDirectory, "C:\\");
    Panel->PanelStateMemory = (u8*)FileViewState;
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
FileView_Render(panel Panel, rect PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context, mouse_state Mouse)
{
    rect HeaderBounds = {0};
    HeaderBounds.Min = {PanelBounds.Min.x, PanelBounds.Max.y - 32};
    HeaderBounds.Max = PanelBounds.Max;
    
    rect ListBounds = {0};
    ListBounds.Min = PanelBounds.Min;
    ListBounds.Max = BottomRight(HeaderBounds);
    
    PushRenderQuad2D(RenderBuffer, RectExpand(HeaderBounds), PinkV4);
    PushRenderQuad2D(RenderBuffer, RectExpand(ListBounds), RedV4);
}



#define FOLDHAUS_PANEL_FILE_VIEW_H
#endif // FOLDHAUS_PANEL_FILE_VIEW_H
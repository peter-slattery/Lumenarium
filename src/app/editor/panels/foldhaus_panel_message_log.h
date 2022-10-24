/* date = April 12th 2021 4:47 pm */

#ifndef FOLDHAUS_PANEL_MESSAGE_LOG_H
#define FOLDHAUS_PANEL_MESSAGE_LOG_H

GSMetaTag(panel_init);
GSMetaTag(panel_type_file_view);
internal void
MessageLog_Init(panel* Panel, app_state* State, context Context)
{
}

GSMetaTag(panel_cleanup);
GSMetaTag(panel_type_file_view);
internal void
MessageLog_Cleanup(panel* Panel, app_state* State)
{
    
}

GSMetaTag(panel_render);
GSMetaTag(panel_type_file_view);
internal void
MessageLog_Render(panel* Panel, rect2 PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    ui_interface* Interface = &State->Interface;
    ui_widget* Layout = ui_PushLayout(Interface, PanelBounds, LayoutDirection_TopDown, MakeString("Message Log Layout"));
    
    ui_BeginList(Interface, MakeString("Message Log List"), 10, GlobalLogBuffer->EntriesCount);
    
    log_buffer_iter Iter = Log_GetIter(GlobalLogBuffer);
    while (true)
    {
        log_entry* At = Iter.At;
        ui_Label(Interface, At->String);
        if (!LogIter_CanAdvance(Iter))
        {
            break;
        }
        LogIter_Advance(&Iter);
    }
    ui_EndList(Interface);
    
    ui_PopLayout(Interface, MakeString("Message Log Layout"));
}
#endif //FOLDHAUS_PANEL_MESSAGE_LOG_H

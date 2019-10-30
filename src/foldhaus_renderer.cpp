internal render_command_buffer
AllocateRenderCommandBuffer (u8* Memory, s32 Size)
{
    render_command_buffer Result = {};
    Result.CommandMemory = Memory;
    Result.CommandMemoryUsed = 0;
    Result.CommandMemorySize = Size;
    return Result;
}

internal void
Render3DQuadBatch (u8* CommandData, s32 TriCount)
{
    DEBUG_TRACK_FUNCTION;
    
    v4* Vertecies = (v4*)(CommandData + BATCH_3D_VERTECIES_OFFSET(TriCount));
    v2* UVs = (v2*)(CommandData + BATCH_3D_UVS_OFFSET(TriCount));
    v4* Colors = (v4*)(CommandData + BATCH_3D_COLORS_OFFSET(TriCount));
    
#if IMMEDIATE_MODE_RENDERING
    
    for (s32 Tri = 0; Tri < TriCount; Tri++)
    {
        v4 P0 = Vertecies[BATCH_3D_VERTEX_INDEX(Tri, 0)];
        v4 P1 = Vertecies[BATCH_3D_VERTEX_INDEX(Tri, 1)];
        v4 P2 = Vertecies[BATCH_3D_VERTEX_INDEX(Tri, 2)];
        v2 UV0 = UVs[BATCH_3D_UV_INDEX(Tri, 0)];
        v2 UV1 = UVs[BATCH_3D_UV_INDEX(Tri, 1)];
        v2 UV2 = UVs[BATCH_3D_UV_INDEX(Tri, 2)];
        v4 C0 = Colors[BATCH_3D_COLOR_INDEX(Tri, 0)];
        v4 C1 = Colors[BATCH_3D_COLOR_INDEX(Tri, 1)];
        v4 C2 = Colors[BATCH_3D_COLOR_INDEX(Tri, 2)];
        
        OpenGLDraw3DTri(P0, P1, P2, UV0, UV1, UV2, C0, C1, C2);
    }
#else
    OpenGLRenderTriBuffer((u8*)Vertecies, 4, (u8*)UVs, 2, (u8*)Colors, 4, TriCount * 3);
#endif
}

internal void
Render2DQuadBatch (u8* CommandData, s32 QuadCount)
{
    DEBUG_TRACK_FUNCTION;
    
    v2* Vertecies = (v2*)(CommandData + BATCH_2D_VERTECIES_OFFSET(QuadCount));
    v2* UVs = (v2*)(CommandData + BATCH_2D_UVS_OFFSET(QuadCount));
    v4* Colors = (v4*)(CommandData + BATCH_2D_COLORS_OFFSET(QuadCount));
    
#if IMMEDIATE_MODE_RENDERING
    for (s32 Quad = 0; Quad < QuadCount; Quad++)
    {
        for (s32 Tri = 0; Tri < 2; Tri++)
        {
            v2 P0 = Vertecies[BATCH_2D_VERTEX_INDEX(Quad, Tri, 0)];
            v2 P1 = Vertecies[BATCH_2D_VERTEX_INDEX(Quad, Tri, 1)];
            v2 P2 = Vertecies[BATCH_2D_VERTEX_INDEX(Quad, Tri, 2)];
            v2 UV0 = UVs[BATCH_2D_UV_INDEX(Quad, Tri, 0)];
            v2 UV1 = UVs[BATCH_2D_UV_INDEX(Quad, Tri, 1)]; 
            v2 UV2 = UVs[BATCH_2D_UV_INDEX(Quad, Tri, 2)];
            v4 C0 = Colors[BATCH_2D_COLOR_INDEX(Quad, Tri, 0)];
            v4 C1 = Colors[BATCH_2D_COLOR_INDEX(Quad, Tri, 1)];
            v4 C2 = Colors[BATCH_2D_COLOR_INDEX(Quad, Tri, 2)];
            
            OpenGLDraw2DTri(P0, P1, P2, UV0, UV1, UV2, C0, C1, C2);
        }
    }
#else
    OpenGLRenderTriBuffer((u8*)Vertecies, 2, (u8*)UVs, 2, (u8*)Colors, 4, QuadCount * 2 * 3);
#endif
}

internal void
RenderCommandBuffer (render_command_buffer CommandBuffer)
{
    DEBUG_TRACK_FUNCTION;
    
    glMatrixMode(GL_TEXTURE_2D);
    glLoadIdentity();
    
    glClearColor(0.1f, 0.1f, 0.1f, 1);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glDisable(GL_TEXTURE_2D);
    b32 GLTextureEnabled = false;
    
    u8* CurrentPosition = CommandBuffer.CommandMemory;
    while(CurrentPosition < CommandBuffer.CommandMemory + CommandBuffer.CommandMemoryUsed)
    {
        render_command_header* CommandHeader = (render_command_header*)CurrentPosition;
        CurrentPosition += sizeof(render_command_header);
        switch (CommandHeader->Type)
        {
            case RenderCommand_render_command_set_render_mode:
            {
                render_command_set_render_mode* Command = (render_command_set_render_mode*)(CommandHeader + 1);
                
                glViewport(Command->ViewOffsetX, Command->ViewOffsetY, 
                           Command->ViewWidth, Command->ViewHeight);
                
                LoadModelView(Command->ModelView.E);
                LoadProjection(Command->Projection.E);
                
                if (Command->UseDepthBuffer)
                {
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_LESS);
                }
                else
                {
                    glDisable(GL_DEPTH_TEST);
                }
                
                CurrentPosition += sizeof(render_command_set_render_mode);
            }break;
            
            case RenderCommand_render_command_clear_screen:
            {
                render_command_clear_screen* Command = (render_command_clear_screen*)(CommandHeader + 1);
                
                ClearRenderBuffer();
                
                CurrentPosition += sizeof(render_command_clear_screen);
            }break;
            
            case RenderCommand_render_batch_command_quad_2d:
            {
                render_batch_command_quad_2d* Command = (render_batch_command_quad_2d*)(CommandHeader + 1);
                
                if (GLTextureEnabled) { glDisable(GL_TEXTURE_2D); GLTextureEnabled = false; }
                u8* CommandData = (u8*)(Command + 1);
                Render2DQuadBatch(CommandData, Command->QuadCount);
                
                CurrentPosition += sizeof(render_batch_command_quad_2d) + Command->DataSize;
            }break;
            
            case RenderCommand_render_batch_command_quad_3d:
            {
                render_batch_command_quad_3d* Command = (render_batch_command_quad_3d*)(CommandHeader + 1);
                
                if (GLTextureEnabled) { glDisable(GL_TEXTURE_2D); GLTextureEnabled = false; }
                u8* CommandData = (u8*)(Command + 1);
                Render3DQuadBatch(CommandData, Command->QuadCount * 2);
                
                CurrentPosition += sizeof(render_batch_command_quad_3d) + Command->DataSize;
            }break;
            
            case RenderCommand_render_batch_command_texture_2d:
            {
                render_batch_command_texture_2d* Command = (render_batch_command_texture_2d*)(CommandHeader + 1);
                
                if (!GLTextureEnabled) { glEnable(GL_TEXTURE_2D); GLTextureEnabled = true; }
                Assert(Command->Texture.Handle > 0);
                glBindTexture(GL_TEXTURE_2D, Command->Texture.Handle);
                u8* CommandData = (u8*)(Command + 1);
                Render2DQuadBatch(CommandData, Command->QuadCount);
                
                CurrentPosition += sizeof(render_batch_command_texture_2d) + Command->DataSize;
            }break;
            
            default:
            {
                InvalidCodePath;
            }break;
        }
    }
}

internal void
ClearRenderBuffer (render_command_buffer* Buffer)
{
    Buffer->CommandMemoryUsed = 0;
}
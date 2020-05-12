//
// File: foldhaus_renderer.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_RENDERER_H

#define IMMEDIATE_MODE_RENDERING 0

struct camera
{
    r32 FieldOfView;
    r32 AspectRatio;
    r32 Near, Far;
    v3 Position;
    v3 LookAt;
};

inline m44
GetCameraModelViewMatrix (camera Camera)
{
    // Forward
    v4 CamForward = V4(Normalize(Camera.Position - Camera.LookAt), 0); 
    // Right
    v4 CamRight = Normalize(Cross(v4{0, 1, 0, 0}, CamForward));
    // Up
    v4 CamUp = Normalize(Cross(CamForward, CamRight));
    
    r32 X = Camera.Position.x;
    r32 Y = Camera.Position.y;
    r32 Z = Camera.Position.z;
    
    m44 RotationMatrix = M44(
                             CamRight.x, CamUp.x, CamForward.x, 0,
                             CamRight.y, CamUp.y, CamForward.y, 0,
                             CamRight.z, CamUp.z, CamForward.z, 0,
                             0,       0,    0,         1);
    
    m44 PositionMatrix = M44(
                             1, 0, 0, 0,
                             0, 1, 0, 0,
                             0, 0, 1, 0,
                             -X, -Y, -Z, 1
                             );
    
    m44 ModelViewMatrix = PositionMatrix * RotationMatrix;
    
    return ModelViewMatrix;
}

inline m44
GetCameraPerspectiveProjectionMatrix(camera Camera)
{
    r32 Top = Camera.Near * GSTan((Camera.FieldOfView / 2.0f));
    r32 Bottom = -Top;
    r32 Right = Top * Camera.AspectRatio;
    r32 Left = -Right;
    
    r32 A = ((Right + Left) / (Right - Left));
    r32 B = ((Top + Bottom) / (Top - Bottom));
    r32 C = -((Camera.Far + Camera.Near) / (Camera.Far - Camera.Near));
    r32 D = -((2 * Camera.Far * Camera.Near) / (Camera.Far - Camera.Near));
    
    r32 E = ((2 * Camera.Near) / (Right - Left));
    r32 F = ((2 * Camera.Near) / (Top - Bottom));
    
    m44 PerspectiveProjectionMatrix  = 
    {
        E,  0,  A,  0,
        0,  F,  B,  0,
        0,  0,  C,  D,
        0,  0, -1,  0
    };
    
    return PerspectiveProjectionMatrix;
}

// Render Commands
// Discriminated Union
enum render_command_type
{
    RenderCommand_Invalid,
    
    RenderCommand_render_command_clear_screen,
    RenderCommand_render_command_set_render_mode,
    
    RenderCommand_render_batch_command_quad_2d,
    RenderCommand_render_batch_command_quad_3d,
    RenderCommand_render_batch_command_texture_2d,
    
    RenderCommand_render_command_texture_3d,
    
    RenderCommand_Count
};

struct render_command_header
{
    render_command_type Type;
};

// NOTE(Peter): Just to keep with the rest of the system
struct render_command_clear_screen {};

struct render_quad_2d
{
    v2 Min, Max;
};

struct render_quad_3d
{
    v4 P0, P1, P2, P3;
};

struct render_texture
{
    // TODO(Peter): Is all this necessary?
    u8* Memory;
    s32 Handle;
    s32 Width;
    s32 Height;
    s32 BytesPerPixel;
    s32 Stride;
};

#define BATCH_3D_SIZE(tricount) (((sizeof(v4) + sizeof(v2) + sizeof(v4)) * 3) * tricount)
#define BATCH_3D_VERTECIES_OFFSET(tricount) (0 * tricount)
#define BATCH_3D_UVS_OFFSET(tricount) (BATCH_3D_VERTECIES_OFFSET(tricount) + ((sizeof(v4) * 3) * tricount))
#define BATCH_3D_COLORS_OFFSET(tricount) (BATCH_3D_UVS_OFFSET(tricount) + ((sizeof(v2) * 3) * tricount))
#define BATCH_3D_VERTEX_INDEX(tri, v) ((tri * 3) + v)
#define BATCH_3D_UV_INDEX(tri, v) ((tri * 3) + v)
#define BATCH_3D_COLOR_INDEX(tri, v) ((tri * 3) + v)

#define BATCH_2D_SIZE(quadcount) (((sizeof(v2) + sizeof(v2) + sizeof(v4)) * 3) * 2 * quadcount)
#define BATCH_2D_VERTECIES_OFFSET(quadcount) (0 * quadcount)
#define BATCH_2D_UVS_OFFSET(quadcount) (BATCH_2D_VERTECIES_OFFSET(quadcount) + ((sizeof(v2) * 3) * 2 * quadcount))
#define BATCH_2D_COLORS_OFFSET(quadcount) (BATCH_2D_UVS_OFFSET(quadcount) + ((sizeof(v2) * 3) * 2 * quadcount))
#define BATCH_2D_VERTEX_INDEX(quad, tri, v) ((quad * 6) + (tri * 3) + v)
#define BATCH_2D_UV_INDEX(quad, tri, v) ((quad * 6) + (tri * 3) + v)
#define BATCH_2D_COLOR_INDEX(quad, tri, v) ((quad * 6) + (tri * 3) + v)

struct render_quad_batch_constructor
{
    s32 Max;
    s32 Count;
    
    v4* Vertecies;
    v2* UVs;
    v4* ColorsV;
};

struct render_batch_command_quad_2d
{
    s32 QuadCount;
    s32 DataSize;
    // NOTE(Peter): The data immediately follows the command in memory
};

struct render_batch_command_quad_3d
{
    s32 QuadCount;
    s32 DataSize;
    // NOTE(Peter): The data immediately follows the command in memory
};

struct render_command_texture_2d
{
    render_quad_2d Quad;
    render_quad_2d UV;
    v4 Color;
    render_texture Texture;
};

struct render_batch_command_texture_2d
{
    s32 QuadCount;
    s32 DataSize;
    render_texture Texture;
};

struct render_command_texture_3d
{
    render_quad_3d Quad;
    v4 Color;
    render_texture Texture;
};

struct render_command_set_render_mode
{
    m44 ModelView;
    m44 Projection;
    r32 ViewOffsetX, ViewOffsetY;
    r32 ViewWidth, ViewHeight;
    b32 UseDepthBuffer;
};

typedef u8* renderer_realloc(u8* Base, s32 CurrentSize, s32 NewSize);

#define COMMAND_BUFFER_MIN_GROW_SIZE Megabytes(2)

struct render_command_buffer
{
    u8* CommandMemory;
    s32 CommandMemoryUsed;
    s32 CommandMemorySize;
    
    renderer_realloc* Realloc;
    
    s32 ViewWidth;
    s32 ViewHeight;
};

///
// Utility
///

internal u32
PackColorStructU8 (u8 R, u8 G, u8 B, u8 A)
{
    u32 Result = (u32)(A << 24 |
                       R << 16 |
                       G << 8  |
                       B<< 0);
    return Result;
}

internal u32
PackColorStructR32 (r32 In_R, r32 In_G, r32 In_B, r32 In_A)
{
    Assert ((In_R >= 0.0f && In_R <= 1.0f) &&
            (In_G >= 0.0f && In_G <= 1.0f) &&
            (In_B >= 0.0f && In_B <= 1.0f) &&
            (In_A >= 0.0f && In_A <= 1.0f));
    
    u8 R = (u8)(255 * In_R);
    u8 G = (u8)(255 * In_G);
    u8 B = (u8)(255 * In_B);
    u8 A = (u8)(255 * In_A);
    
    u32 Result = (u32)(A << 24 |
                       R << 16 |
                       G << 8  |
                       B<< 0);
    return Result;
}

internal void
ResizeBufferIfNecessary(render_command_buffer* Buffer, s32 DataSize)
{
    if (Buffer->CommandMemoryUsed + DataSize > Buffer->CommandMemorySize)
    {
        // NOTE(Peter): If this becomes a problem just go back to the original solution of
        // NewSize =  Buffer->CommandMemorySize + (2 * DataSize);
        s32 SpaceAvailable = Buffer->CommandMemorySize - Buffer->CommandMemoryUsed;
        s32 SpaceNeeded = DataSize - SpaceAvailable; // This is known to be positive at this point
        s32 AdditionSize = GSMax(SpaceNeeded, COMMAND_BUFFER_MIN_GROW_SIZE);
        s32 NewSize = Buffer->CommandMemorySize + AdditionSize;
        Buffer->CommandMemory = Buffer->Realloc(Buffer->CommandMemory, 
                                                Buffer->CommandMemorySize,
                                                NewSize);
        Buffer->CommandMemorySize = NewSize;
    }
}

// Batch

internal s32
PushQuad3DBatch (render_command_buffer* Buffer, render_quad_batch_constructor* Constructor, u8* MemStart, s32 TriCount, s32 DataSize, b32 UseIntegerColor = false)
{
    Constructor->Max = TriCount;
    Constructor->Count = 0;
    
    Constructor->Vertecies = (v4*)(MemStart + BATCH_3D_VERTECIES_OFFSET(TriCount));
    Constructor->UVs = (v2*)(MemStart + BATCH_3D_UVS_OFFSET(TriCount));
    Constructor->ColorsV = (v4*)(MemStart + BATCH_3D_COLORS_OFFSET(TriCount));
    
    Buffer->CommandMemoryUsed += DataSize;
    return DataSize;
}

internal s32
PushQuad2DBatch (render_command_buffer* Buffer, render_quad_batch_constructor* Constructor, s32 QuadCount, s32 DataSize, u8* MemStart)
{
    GSZeroMemory(MemStart, DataSize);
    
    Constructor->Max = QuadCount;
    Constructor->Count = 0;
    
    Constructor->Vertecies = (v4*)(MemStart + BATCH_2D_VERTECIES_OFFSET(QuadCount));
    Constructor->UVs = (v2*)(MemStart + BATCH_2D_UVS_OFFSET(QuadCount));
    Constructor->ColorsV = (v4*)(MemStart + BATCH_2D_COLORS_OFFSET(QuadCount));
    
    Buffer->CommandMemoryUsed += DataSize;
    return DataSize;
}

internal s32
ThreadSafeIncrementQuadConstructorCount (render_quad_batch_constructor* Constructor)
{
    s32 Result = InterlockedIncrement((long*)&Constructor->Count);
    // NOTE(Peter): Have to decrement the value by one. 
    // Interlocked Increment acts as (++Constructor->Count), not (Constructor->Count++) which
    // is what we wanted;
    // This was causing the first triangle to be garbage data. 
    Result -= 1;
    return Result;
}

struct quad_batch_constructor_reserved_range
{
    s32 Start;
    s32 OnePastLast;
};

internal quad_batch_constructor_reserved_range
ThreadSafeReserveRangeInQuadConstructor(render_quad_batch_constructor* Constructor, s32 TrisNeeded)
{
    quad_batch_constructor_reserved_range Result = {};
    Result.OnePastLast = InterlockedAdd((long*)&Constructor->Count, TrisNeeded);
    Result.Start = Result.OnePastLast - TrisNeeded;
    return Result;
}

inline void
SetTri3DInBatch (render_quad_batch_constructor* Constructor, s32 TriIndex,
                 v4 P0, v4 P1, v4 P2, 
                 v2 UV0, v2 UV1, v2 UV2, 
                 v4 C0, v4 C1, v4 C2)
{
    // Vertecies
    Constructor->Vertecies[BATCH_3D_VERTEX_INDEX(TriIndex, 0)] = P0;
    Constructor->Vertecies[BATCH_3D_VERTEX_INDEX(TriIndex, 1)] = P1;
    Constructor->Vertecies[BATCH_3D_VERTEX_INDEX(TriIndex, 2)] = P2;
    
    // UVs
    Constructor->UVs[BATCH_3D_UV_INDEX(TriIndex, 0)] = UV0;
    Constructor->UVs[BATCH_3D_UV_INDEX(TriIndex, 1)] = UV1;
    Constructor->UVs[BATCH_3D_UV_INDEX(TriIndex, 2)] = UV1;
    
    // Color V0
    Constructor->ColorsV[BATCH_3D_COLOR_INDEX(TriIndex, 0)] = C0;
    Constructor->ColorsV[BATCH_3D_COLOR_INDEX(TriIndex, 1)] = C1;
    Constructor->ColorsV[BATCH_3D_COLOR_INDEX(TriIndex, 2)] = C2;
}

inline void
PushTri3DOnBatch (render_quad_batch_constructor* Constructor,  
                  v4 P0, v4 P1, v4 P2, 
                  v2 UV0, v2 UV1, v2 UV2, 
                  v4 C0, v4 C1, v4 C2)
{
    DEBUG_TRACK_FUNCTION;
    s32 Tri = ThreadSafeIncrementQuadConstructorCount(Constructor);
    SetTri3DInBatch(Constructor, Tri, P0, P1, P2, UV0, UV1, UV2, C0, C1, C2);
};

internal void
PushQuad3DOnBatch (render_quad_batch_constructor* Constructor, v4 P0, v4 P1, v4 P2, v4 P3, v2 UVMin, v2 UVMax, v4 Color)
{
    Assert(Constructor->Count + 2 < Constructor->Max);
    PushTri3DOnBatch(Constructor, P0, P1, P2, UVMin, v2{UVMax.x, UVMin.y}, UVMax, Color, Color, Color);
    PushTri3DOnBatch(Constructor, P0, P2, P3, UVMin, UVMax, v2{UVMin.x, UVMax.y}, Color, Color, Color);
}

internal void
PushQuad3DOnBatch (render_quad_batch_constructor* Constructor, 
                   v4 P0, v4 P1, v4 P2, v4 P3, 
                   v2 UV0, v2 UV1, v2 UV2, v2 UV3, 
                   v4 C0, v4 C1, v4 C2, v4 C3)
{
    Assert(Constructor->Count < Constructor->Max);
    PushTri3DOnBatch(Constructor, P0, P1, P2, UV0, UV1, UV2, C0, C1, C2);
    PushTri3DOnBatch(Constructor, P0, P2, P3, UV0, UV2, UV3, C0, C2, C3);
}

internal void
PushQuad3DOnBatch (render_quad_batch_constructor* Constructor, v4 P0, v4 P1, v4 P2, v4 P3, v4 Color)
{
    PushQuad3DOnBatch(Constructor, P0, P1, P2, P3, v2{0, 0}, v2{1, 1}, Color);
}

internal void
PushQuad2DOnBatch (render_quad_batch_constructor* Constructor, 
                   v2 P0, v2 P1, v2 P2, v2 P3, 
                   v2 UV0, v2 UV1, v2 UV2, v2 UV3,
                   v4 C0, v4 C1, v4 C2, v4 C3)
{
    DEBUG_TRACK_FUNCTION;
    
    s32 Quad = ThreadSafeIncrementQuadConstructorCount(Constructor);
    v2* Vertecies = (v2*)Constructor->Vertecies;
    
    // Tri 1
    Vertecies[BATCH_2D_VERTEX_INDEX(Quad, 0, 0)] = P0;
    Vertecies[BATCH_2D_VERTEX_INDEX(Quad, 0, 1)] = P1;
    Vertecies[BATCH_2D_VERTEX_INDEX(Quad, 0, 2)] = P2;
    
    // Tri 2
    Vertecies[BATCH_2D_VERTEX_INDEX(Quad, 1, 0)] = P0;
    Vertecies[BATCH_2D_VERTEX_INDEX(Quad, 1, 1)] = P2;
    Vertecies[BATCH_2D_VERTEX_INDEX(Quad, 1, 2)] = P3;
    
    // Tri 1 UVs
    Constructor->UVs[BATCH_2D_UV_INDEX(Quad, 0, 0)] = UV0;
    Constructor->UVs[BATCH_2D_UV_INDEX(Quad, 0, 1)] = UV1;
    Constructor->UVs[BATCH_2D_UV_INDEX(Quad, 0, 2)] = UV2;
    // Tri 2 UVs
    Constructor->UVs[BATCH_2D_UV_INDEX(Quad, 1, 0)] = UV0;
    Constructor->UVs[BATCH_2D_UV_INDEX(Quad, 1, 1)] = UV2;
    Constructor->UVs[BATCH_2D_UV_INDEX(Quad, 1, 2)] = UV3;
    
    // Tri 1 Colors
    Constructor->ColorsV[BATCH_2D_COLOR_INDEX(Quad, 0, 0)] = C0;
    Constructor->ColorsV[BATCH_2D_COLOR_INDEX(Quad, 0, 1)] = C1;
    Constructor->ColorsV[BATCH_2D_COLOR_INDEX(Quad, 0, 2)] = C2;
    // Tri 2 Colors
    Constructor->ColorsV[BATCH_2D_COLOR_INDEX(Quad, 1, 0)] = C0;
    Constructor->ColorsV[BATCH_2D_COLOR_INDEX(Quad, 1, 1)] = C2;
    Constructor->ColorsV[BATCH_2D_COLOR_INDEX(Quad, 1, 2)] = C3;
}

internal void
PushQuad2DOnBatch (render_quad_batch_constructor* Constructor, v2 P0, v2 P1, v2 P2, v2 P3, v2 UVMin, v2 UVMax, v4 Color)
{
    DEBUG_TRACK_FUNCTION;
    
    s32 Quad = ThreadSafeIncrementQuadConstructorCount(Constructor);
    v2* Vertecies = (v2*)Constructor->Vertecies;
    
    // Tri 1
    Vertecies[BATCH_2D_VERTEX_INDEX(Quad, 0, 0)] = P0;
    Vertecies[BATCH_2D_VERTEX_INDEX(Quad, 0, 1)] = P1;
    Vertecies[BATCH_2D_VERTEX_INDEX(Quad, 0, 2)] = P2;
    
    // Tri 2
    Vertecies[BATCH_2D_VERTEX_INDEX(Quad, 1, 0)] = P0;
    Vertecies[BATCH_2D_VERTEX_INDEX(Quad, 1, 1)] = P2;
    Vertecies[BATCH_2D_VERTEX_INDEX(Quad, 1, 2)] = P3;
    
    // Tri 1 UVs
    Constructor->UVs[BATCH_2D_UV_INDEX(Quad, 0, 0)] = UVMin;
    Constructor->UVs[BATCH_2D_UV_INDEX(Quad, 0, 1)] = v2{UVMax.x, UVMin.y};
    Constructor->UVs[BATCH_2D_UV_INDEX(Quad, 0, 2)] = UVMax;
    // Tri 2 UVs
    Constructor->UVs[BATCH_2D_UV_INDEX(Quad, 1, 0)] = UVMin;
    Constructor->UVs[BATCH_2D_UV_INDEX(Quad, 1, 1)] = UVMax;
    Constructor->UVs[BATCH_2D_UV_INDEX(Quad, 1, 2)] = v2{UVMin.x, UVMax.y};
    
    // Tri 1 Colors
    Constructor->ColorsV[BATCH_2D_COLOR_INDEX(Quad, 0, 0)] = Color;
    Constructor->ColorsV[BATCH_2D_COLOR_INDEX(Quad, 0, 1)] = Color;
    Constructor->ColorsV[BATCH_2D_COLOR_INDEX(Quad, 0, 2)] = Color;
    // Tri 2 Colors
    Constructor->ColorsV[BATCH_2D_COLOR_INDEX(Quad, 1, 0)] = Color;
    Constructor->ColorsV[BATCH_2D_COLOR_INDEX(Quad, 1, 1)] = Color;
    Constructor->ColorsV[BATCH_2D_COLOR_INDEX(Quad, 1, 2)] = Color;
}

internal void
PushQuad2DOnBatch (render_quad_batch_constructor* Constructor, v2 Min, v2 Max, v4 Color)
{
    PushQuad2DOnBatch(Constructor, v2{Min.x, Min.y}, v2{Max.x, Min.y}, v2{Max.x, Max.y}, v2{Min.x, Max.y},
                      v2{0, 0}, v2{1, 1}, Color);
}

internal void
PushLine2DOnBatch (render_quad_batch_constructor* Constructor, v2 P0, v2 P1, r32 Thickness, v4 Color)
{
    r32 HalfThickness = Thickness / 2.0f;
    v2 Perpendicular = Normalize(PerpendicularCCW(P1 - P0)) * HalfThickness;
    
    PushQuad2DOnBatch(Constructor, P0 - Perpendicular, P1 - Perpendicular, P1 + Perpendicular, P0 + Perpendicular,
                      v2{0, 0}, v2{1, 1}, Color);
}

// Commands
#define PushRenderCommand(buffer, type) (type*) PushRenderCommand_(buffer, RenderCommand_##type, sizeof(type) + sizeof(render_command_header))

internal u8*
PushRenderCommand_ (render_command_buffer* CommandBuffer, render_command_type CommandType, s32 CommandSize)
{
    ResizeBufferIfNecessary(CommandBuffer, CommandSize);
    Assert(CommandBuffer->CommandMemoryUsed + CommandSize <= CommandBuffer->CommandMemorySize);
    
    render_command_header* Header = (render_command_header*)(CommandBuffer->CommandMemory + CommandBuffer->CommandMemoryUsed);
    Header->Type = CommandType;
    
    u8* Result = (u8*)(Header + 1);
    CommandBuffer->CommandMemoryUsed += CommandSize;
    
    return Result;
}

internal render_command_set_render_mode*
PushRenderPerspective (render_command_buffer* Buffer, s32 OffsetX, s32 OffsetY, s32 ViewWidth, s32 ViewHeight, camera Camera)
{
    render_command_set_render_mode* Command = PushRenderCommand(Buffer, render_command_set_render_mode);
    
    Command->ModelView = GetCameraModelViewMatrix(Camera);
    Command->Projection = GetCameraPerspectiveProjectionMatrix(Camera);
    
    Command->ViewOffsetX = (r32)OffsetX;
    Command->ViewOffsetY = (r32)OffsetY;
    Command->ViewWidth = (r32)ViewWidth;
    Command->ViewHeight = (r32)ViewHeight;
    
    Command->UseDepthBuffer = true;
    
    return Command;
}

internal void
PushRenderOrthographic (render_command_buffer* Buffer, s32 OffsetX, s32 OffsetY, s32 ViewWidth, s32 ViewHeight)
{
    render_command_set_render_mode* Command = PushRenderCommand(Buffer, render_command_set_render_mode);
    Command->ModelView = m44{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    
    r32 a = 2.0f / ViewWidth;
    r32 b = 2.0f / ViewHeight;
    Command->Projection = m44{
        a,   0,  0, 0,
        0,   b,  0, 0,
        0,   0,  1, 0,
        -1, -1,  0, 1
    };
    
    Command->ViewOffsetX = (r32)OffsetX;
    Command->ViewOffsetY = (r32)OffsetY;
    Command->ViewWidth = ViewWidth;
    Command->ViewHeight = ViewHeight;
    
    Command->UseDepthBuffer = false;;
}

internal void
PushRenderClearScreen (render_command_buffer* Buffer)
{
    render_command_clear_screen* Command = PushRenderCommand(Buffer, render_command_clear_screen);
}

internal render_quad_batch_constructor
PushRenderQuad2DBatch(render_command_buffer* Buffer, s32 QuadCount)
{
    s32 DataSize = BATCH_2D_SIZE(QuadCount);
    ResizeBufferIfNecessary(Buffer, DataSize + sizeof(render_batch_command_quad_2d));
    Assert(Buffer->CommandMemoryUsed + DataSize <= Buffer->CommandMemorySize);
    
    render_quad_batch_constructor Result = {};
    
    render_batch_command_quad_2d* Command = PushRenderCommand(Buffer, render_batch_command_quad_2d);
    Command->QuadCount = QuadCount;
    Command->DataSize = PushQuad2DBatch(Buffer, &Result, QuadCount, DataSize, (u8*)(Command + 1));
    
    return Result;
}

internal void
PushRenderQuad2D (render_command_buffer* Buffer, v2 Min, v2 Max, v4 Color)
{
    render_quad_batch_constructor Batch = PushRenderQuad2DBatch(Buffer, 1);
    PushQuad2DOnBatch(&Batch, Min, Max, Color);
}

internal void
PushRenderLine2D (render_command_buffer* Buffer, v2 P0, v2 P1, r32 Thickness, v4 Color)
{
    render_quad_batch_constructor Batch = PushRenderQuad2DBatch(Buffer, 1);
    PushLine2DOnBatch(&Batch, P0, P1, Thickness, Color);
}


internal render_quad_batch_constructor
PushRenderQuad3DBatch(render_command_buffer* Buffer, s32 QuadCount)
{
    s32 TriCount = QuadCount * 2;
    s32 DataSize = BATCH_3D_SIZE(TriCount);
    ResizeBufferIfNecessary(Buffer, DataSize + sizeof(render_batch_command_quad_3d));
    Assert(Buffer->CommandMemoryUsed + DataSize <= Buffer->CommandMemorySize);
    
    render_quad_batch_constructor Result = {};
    
    render_batch_command_quad_3d* Command = PushRenderCommand(Buffer, render_batch_command_quad_3d);
    Command->QuadCount = QuadCount;
    Command->DataSize = PushQuad3DBatch(Buffer, &Result, (u8*)(Command + 1), TriCount, DataSize);
    
    return Result;
}

internal void
PushRenderQuad3D (render_command_buffer* Buffer, v4 A, v4 B, v4 C, v4 D, v4 Color)
{
    render_quad_batch_constructor Batch = PushRenderQuad3DBatch(Buffer, 1);
    PushQuad3DOnBatch(&Batch, A, B, C, D, Color);
}

internal void
PushRenderCameraFacingQuad (render_command_buffer* Buffer, v4 Center, v2 Dimensions, v4 Color)
{
    // TODO(Peter): Turn this into an actual camera facing quad
    v4 A = v4{Center.x - Dimensions.x, Center.y - Dimensions.y, Center.z, Center.w};
    v4 B = v4{Center.x + Dimensions.x, Center.y - Dimensions.y, Center.z, Center.w};
    v4 C = v4{Center.x + Dimensions.x, Center.y + Dimensions.y, Center.z, Center.w};
    v4 D = v4{Center.x - Dimensions.x, Center.y + Dimensions.y, Center.z, Center.w};
    
    PushRenderQuad3D(Buffer, A, B, C, D, Color);
}

internal render_quad_batch_constructor
PushRenderTexture2DBatch(render_command_buffer* Buffer, s32 QuadCount, 
                         render_texture Texture)
{
    s32 DataSize = BATCH_2D_SIZE(QuadCount);
    ResizeBufferIfNecessary(Buffer, DataSize);
    Assert(Buffer->CommandMemoryUsed + DataSize <= Buffer->CommandMemorySize);
    
    render_quad_batch_constructor Result = {};
    
    render_batch_command_texture_2d* Command = PushRenderCommand(Buffer, render_batch_command_texture_2d);
    Command->QuadCount = QuadCount;
    Command->DataSize = PushQuad2DBatch(Buffer, &Result, QuadCount, DataSize, (u8*)(Command + 1));
    Command->Texture = Texture;
    
    return Result;
}

internal render_quad_batch_constructor
PushRenderTexture2DBatch (render_command_buffer* Buffer, s32 QuadCount, 
                          u8* TextureMemory, s32 TextureHandle, s32 TextureWidth, s32 TextureHeight,
                          s32 TextureBytesPerPixel, s32 TextureStride)
{
    render_texture Texture = render_texture{
        TextureMemory, 
        TextureHandle, 
        TextureWidth, 
        TextureHeight, 
        TextureBytesPerPixel,
        TextureStride};
    return PushRenderTexture2DBatch(Buffer, QuadCount, Texture);
}

internal void
PushRenderTexture2D (render_command_buffer* Buffer, v2 Min, v2 Max, v4 Color,
                     v2 UVMin, v2 UVMax,
                     render_texture* Texture)
{
    render_quad_batch_constructor Batch = PushRenderTexture2DBatch(Buffer, 1, *Texture);
    PushQuad2DOnBatch(&Batch, v2{Min.x, Min.y}, v2{Max.x, Min.y}, v2{Max.x, Max.y}, v2{Min.x, Max.y},
                      UVMin, UVMax, Color);
}

internal void
PushRenderBoundingBox2D (render_command_buffer* Buffer, v2 Min, v2 Max, r32 Thickness, v4 Color)
{
    render_quad_batch_constructor Batch = PushRenderQuad2DBatch(Buffer, 4);
    PushQuad2DOnBatch(&Batch, Min, v2{Min.x + Thickness, Max.y}, Color);
    PushQuad2DOnBatch(&Batch, v2{Min.x, Max.y - Thickness}, Max, Color);
    PushQuad2DOnBatch(&Batch, v2{Max.x - Thickness, Min.y}, Max, Color);
    PushQuad2DOnBatch(&Batch, Min, v2{Max.x, Min.y + Thickness}, Color);
}


#define FOLDHAUS_RENDERER_H
#endif // FOLDHAUS_RENDERER_H
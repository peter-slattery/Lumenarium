#ifndef GS_FONT_H

struct codepoint_bitmap
{
    s32 XOffset, YOffset;
    s32 Width, Height;
    s32 BitmapX, BitmapY;
};

struct bitmap_font
{
    char* Name;
    s32 PixelHeight;
    s32 Ascent;
    s32 Descent;
    s32 Leading;
    s32 MaxCharWidth;
    
    u32 CodepointDictionarySize;
    u32 CodepointDictionaryCount;
    char* CodepointKeys;
    codepoint_bitmap* CodepointValues;
    
    u8* BitmapMemory;
    u32 BitmapWidth;
    u32 BitmapHeight;
    u32 BitmapBytesPerPixel;
    u32 BitmapStride;
    s32 BitmapTextureHandle;
};

internal bitmap_font
InitializeTextFont (s32 CodepointCount, u8* CodepointMemory, s32 CodepointMemorySize)
{
    bitmap_font Result = {};
    
    Result.CodepointDictionarySize = CodepointCount;
    Result.CodepointDictionaryCount = 0;
    Assert(CodepointMemorySize >= (sizeof(char) + sizeof(codepoint_bitmap)) * CodepointCount);
    Result.CodepointKeys = (char*)CodepointMemory;
    Result.CodepointValues = (codepoint_bitmap*)(CodepointMemory + (sizeof(char) * CodepointCount));
}

#define GLYPH_SKIRT 1
internal void
GetNextCodepointOffset (bitmap_font* Font, u32* X, u32* Y)
{
    s32 LastCodepointIndex = Font->CodepointDictionaryCount - 1;
    if (LastCodepointIndex >= 0)
    {
        codepoint_bitmap BitmapInfo = Font->CodepointValues[LastCodepointIndex];
        u32 TempX = BitmapInfo.XOffset + BitmapInfo.Width + GLYPH_SKIRT;
        u32 TempY = BitmapInfo.YOffset;
        if (TempX + Font->MaxCharWidth > Font->BitmapWidth)
        {
            TempX = 0;
            TempY -= BitmapInfo.Height + GLYPH_SKIRT;
        }
        *X = TempX;
        *Y = TempY;
    }
    else
    {
        *X = 0;
        *Y = 0;
    }
}

internal void
AddCodepointToFont (bitmap_font* Font, char Codepoint, 
                    s32 XOffset, s32 YOffset, s32 Width, s32 Height,
                    s32 BitmapX, s32 BitmapY)
{
    Assert(Font->CodepointDictionaryCount < Font->CodepointDictionarySize);
    
    char* Key = Font->CodepointKeys + Font->CodepointDictionaryCount;
    codepoint_bitmap* Value = Font->CodepointValues + Font->CodepointDictionaryCount;
    Font->CodepointDictionaryCount++;
    
    *Key = Codepoint;
    *Value = {XOffset, YOffset, Width, Height, BitmapX, BitmapY};
}

internal s32
GetIndexForCodepoint (bitmap_font Font, char Codepoint)
{
    s32 Result = -1;
    for (u32 i = 0; i < Font.CodepointDictionaryCount; i++)
    {
        if (Font.CodepointKeys[i] == Codepoint)
        {
            Result = i;
            break;
        }
    }
    return Result;
}

inline s32
NewLineYOffset (bitmap_font Font)
{
    s32 Result = Font.Ascent - Font.Descent;
    return Result;
}

#define GS_FONT_H
#endif
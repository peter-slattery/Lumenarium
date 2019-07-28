#ifndef GS_WIN32_H

struct platform_font_info
{
    s32 PixelHeight;
    s32 Ascent, Descent, Leading;
    s32 MaxCharWidth;
    s32 CodepointStart;
    s32 CodepointOnePastLast;
};

#define GS_WIN32_H
#endif // GS_WIN32_H

//
// File: foldhaus_text_entry.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_TEXT_ENTRY_H

enum text_translation_type
{
    TextTranslateTo_String,
    TextTranslateTo_R32,
    TextTranslateTo_S32,
    TextTranslateTo_U32,
};

struct text_entry_destination
{
    text_translation_type Type;
    union {
        string* StringDest;
        r32* FloatDest;
        s32* SignedIntDest;
        u32* UnsignedIntDest;
    };
};

struct text_entry
{
    string Buffer;
    s32 CursorPosition;
    
    text_entry_destination Destination;
};

#define FOLDHAUS_TEXT_ENTRY_H
#endif // FOLDHAUS_TEXT_ENTRY_H
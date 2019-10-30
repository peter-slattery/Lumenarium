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
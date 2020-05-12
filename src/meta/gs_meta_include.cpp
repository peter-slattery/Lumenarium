//
// File: gs_meta_include.cpp
// Author: Peter Slattery
// Creation Date: 2020-02-22
//
#ifndef GS_META_INCLUDE_CPP

static gsm_s32
gsm_GetMetaTagIndex(gsm_s32 Needle, gsm_meta_tag_type* Tags, gsm_u32 TagCount)
{
    gsm_s32 Result = -1;
    for (gsm_u32 i = 0; i < TagCount; i++)
    {
        if (Needle == Tags[i])
        {
            Result = (gsm_s32)i;
            break;
        }
    }
    return Result;
}



#define GS_META_INCLUDE_CPP
#endif // GS_META_INCLUDE_CPP
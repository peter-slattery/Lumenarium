//
// File: foldhaus_meta_include.h
// Author: Peter Slattery
// Creation Date: 2020-01-19
//
#ifndef FOLDHAUS_META_INCLUDE_H

//
// NOTE
// Include this file FIRST in any application utilizing the Foldhaus Meta system
// Include the generated files you wish to take advantage of at convenient locations
// in your application
//

typedef int gsm_s32;
typedef unsigned int gsm_u32;
typedef unsigned long long int gsm_u64;
typedef enum gsm_meta_tag_type gsm_meta_tag_type;
typedef enum gsm_struct_type gsm_struct_type;

#define GSMetaTag(ident, ...) 

struct gsm_meta_tag
{
    char* Tag;
    gsm_u32 TagLength;
};

struct gsm_struct_member_type_info
{
    char* Identifier;
    gsm_u32 IdentifierLength;
    gsm_u64 Offset;
    
    // NOTE(Peter): There's no reason that this is 5 other than that its more tags
    // than I need at the moment of writing. It does need to be a static array size
    // because theres no way to statically initialize a pointer to an array
    gsm_meta_tag_type Tags[5];
    gsm_u32 TagsCount;
};

struct gsm_struct_type_info
{
    gsm_u32 Type;
    char* Identifier;
    gsm_u32 IdentifierLength;
    
    gsm_u32 Size;
    
    gsm_meta_tag* Tags;
    gsm_u32 TagsCount;
    
    gsm_struct_member_type_info* Members;
    gsm_u32 MembersCount;
};

static bool
gsm_CharArraysEqual(char* A, char* B)
{
    bool Result = true;
    
    char* AAt = A;
    char* BAt = B;
    
    while (*AAt && *BAt)
    {
        if (*AAt != *BAt)
        {
            Result = false;
            break;
        }
    }
    
    // NOTE(Peter): In case we get to the end of A or B, but not both.
    // ie. the strings are equal up to a point, but one is longer.
    if (*AAt != *BAt)
    {
        Result = false;
    }
    
    return Result;
}

static gsm_s32 gsm_GetMetaTagIndex(gsm_s32 Needle, gsm_meta_tag_type* Tags, gsm_u32 TagsCount);

#define FOLDHAUS_META_INCLUDE_H
#endif // FOLDHAUS_META_INCLUDE_H
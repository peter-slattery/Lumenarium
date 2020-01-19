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

typedef unsigned int gsm_u32;
typedef unsigned long long int gsm_u64;

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


#define FOLDHAUS_META_INCLUDE_H
#endif // FOLDHAUS_META_INCLUDE_H
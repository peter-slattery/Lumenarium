//
// File: gs_meta_typeinfo_generator.h
// Author: Peter Slattery
// Creation Date: 2020-01-19
//
// Usage 
// TODO
// 
//
#ifndef GS_META_TYPEINFO_GENERATOR_H

#include "..\gs_libs\gs_language.h"
#include "..\gs_libs\gs_string.h"
#include "..\gs_libs\gs_string_builder.h"
#include "gs_meta_code_generator.h"

struct typeinfo_generator
{
    string_builder MetaTagString;
    string_builder MetaTagEnum;
    
    string_builder TypeListString;
    gsm_code_generator TypeList;
    string_builder StructMembers;
    string_builder TypeDefinitions;
    
    u32 GeneratedInfoTypesCount;
    u32 TypesMax;
    b8* TypesGeneratedMask;
};

#define TypeHandleToIndex(handle) ((handle.BucketIndex * TYPE_TABLE_BUCKET_MAX) + handle.IndexInBucket)
internal typeinfo_generator
InitTypeInfoGenerator(type_table TypeTable)
{
    typeinfo_generator Result = {};
    
    Result.TypesMax = TypeTable.TypeBucketsCount * TYPE_TABLE_BUCKET_MAX;
    Result.TypesGeneratedMask = (b8*)malloc(sizeof(b8) * Result.TypesMax);
    GSZeroMemory((u8*)Result.TypesGeneratedMask, Result.TypesMax);
    
    Result.TypeList = BeginEnumGeneration("gsm_struct_type", "gsm_StructType", false, true);
    
    WriteF(&Result.TypeDefinitions, "static gsm_struct_type_info StructTypes[] = {\n");
    return Result;
}

internal void
FinishGeneratingTypes(typeinfo_generator* Generator)
{
    FinishEnumGeneration(&Generator->TypeList);
    
    WriteF(&Generator->StructMembers, "\n");
    
    WriteF(&Generator->TypeDefinitions, "};\n");
    WriteF(&Generator->TypeDefinitions, "static gsm_u32 StructTypesCount = %d;\n", Generator->GeneratedInfoTypesCount);
}

internal void
GenerateMetaTagInfo (gs_bucket<type_table_handle> Tags, type_table TypeTable, string_builder* Builder)
{
    WriteF(Builder, "{");
    for (u32 t = 0; t < Tags.Used; t++)
    {
        type_table_handle TagHandle = *Tags.GetElementAtIndex(t);
        meta_tag* Tag = GetMetaTag(TagHandle, TypeTable);
        WriteF(Builder, "MetaTag_%S, ", Tag->Identifier);
    }
    WriteF(Builder, "}, %d", Tags.Used);
}

internal void
GenerateStructMemberInfo (variable_decl* Member, string StructIdentifier, type_table TypeTable, typeinfo_generator* Gen)
{
    WriteF(&Gen->StructMembers, "{ \"%S\", %d, ", Member->Identifier, Member->Identifier.Length);
    WriteF(&Gen->StructMembers, "(u64)&((%S*)0)->%S, ", StructIdentifier, Member->Identifier); 
    GenerateMetaTagInfo(Member->MetaTags, TypeTable, &Gen->StructMembers);
    WriteF(&Gen->StructMembers, "},\n");
}

internal void
GenerateTypeInfo (type_definition* Type, type_table_handle TypeHandle, type_table TypeTable, typeinfo_generator* Generator)
{
    // TODO(Peter): 
    // 1. allocate the full range of the types hash table
    // 2. use bucketindex * bucket_max + indexinbucket to get the consecutive index
    Generator->TypesGeneratedMask[TypeHandleToIndex(TypeHandle)] = true;
    Generator->GeneratedInfoTypesCount++;
    
    {
        // NOTE(Peter): This block MUST come before generating
        // type info for any member types. If it doesn't, it will screw
        // up array ordering
        
        AddEnumElement(&Generator->TypeList, Type->Identifier);
        
        // Type Info
        WriteF(&Generator->TypeDefinitions, "{ gsm_StructType_%S, \"%S\", %d, %d, 0, 0, ",
               Type->Identifier,
               Type->Identifier, Type->Identifier.Length,
               Type->Size
               // TODO(Peter): include Meta Tags somehow
               );
        if ((Type->Type == TypeDef_Struct || Type->Type == TypeDef_Union) &&
            Type->Struct.MemberDecls.Used > 0)
        {
            WriteF(&Generator->TypeDefinitions, "StructMembers_%S, %d },\n",
                   Type->Identifier,
                   Type->Struct.MemberDecls.Used);
        }
        else
        {
            WriteF(&Generator->TypeDefinitions, "0, 0 },\n");
        }
    }
    
    if (Type->Type == TypeDef_Struct || 
        Type->Type == TypeDef_Union)
    {
        for (u32 m = 0; m < Type->Struct.MemberDecls.Used; m++)
        {
            variable_decl* Member = Type->Struct.MemberDecls.GetElementAtIndex(m);
            type_definition* MemberType = GetTypeDefinition(Member->TypeHandle, TypeTable);
            
            if ((MemberType->Type == TypeDef_Struct ||
                 MemberType->Type == TypeDef_Union) &&
                MemberType->Struct.IsAnonymous) 
            { 
                continue; // Don't gen info for anonymous struct and union members
            } 
            
            if (Generator->TypesGeneratedMask[TypeHandleToIndex(Member->TypeHandle)]) { continue; }
            
            GenerateTypeInfo(MemberType, Member->TypeHandle, TypeTable, Generator);
        }
        
        //
        WriteF(&Generator->StructMembers, "static gsm_struct_member_type_info StructMembers_%S[] = {\n", Type->Identifier);
        for (u32 m = 0; m < Type->Struct.MemberDecls.Used; m++)
        {
            variable_decl* Member = Type->Struct.MemberDecls.GetElementAtIndex(m);
            type_definition* MemberType = GetTypeDefinition(Member->TypeHandle, TypeTable);
            
            if ((MemberType->Type != TypeDef_Struct && MemberType->Type != TypeDef_Union) ||
                !MemberType->Struct.IsAnonymous)
            {
                GenerateStructMemberInfo(Member, Type->Identifier, TypeTable, Generator);
            }
            else if (MemberType->Struct.IsAnonymous)
            {
                // Anonymous Members
                for (u32 a = 0; a < MemberType->Struct.MemberDecls.Used; a++)
                {
                    variable_decl* AnonMember = MemberType->Struct.MemberDecls.GetElementAtIndex(a);
                    GenerateStructMemberInfo(AnonMember, Type->Identifier, TypeTable, Generator);
                }
            }
        }
        WriteF(&Generator->StructMembers, "};\n", Type->Struct.MemberDecls.Used);
    }
}

internal void
GenerateFilteredTypeInfo (string MetaTagFilter, type_table TypeTable, typeinfo_generator* Generator)
{
    for (u32 b = 0; b < TypeTable.TypeBucketsCount; b++)
    {
        type_table_hash_bucket Bucket = TypeTable.Types[b];
        
        for (u32 i = 0; i < TYPE_TABLE_BUCKET_MAX; i++)
        {
            type_table_handle TypeHandle = {};
            TypeHandle.BucketIndex = b;
            TypeHandle.IndexInBucket = i;
            
            if (Generator->TypesGeneratedMask[TypeHandleToIndex(TypeHandle)])
            {
                continue;
            }
            
            type_definition* Type = GetTypeDefinitionUnsafe(TypeHandle, TypeTable);
            if (Type)
            {
                if (HasTag(MetaTagFilter, Type->MetaTags, TypeTable))
                {
                    GenerateTypeInfo(Type, TypeHandle, TypeTable, Generator);
                }
            }
        }
    }
}

internal void
GenerateMetaTagList(type_table TypeTable, typeinfo_generator* Generator)
{
    WriteF(&Generator->MetaTagString, "gsm_meta_tag MetaTagStrings[] = {\n");
    WriteF(&Generator->MetaTagEnum, "enum gsm_meta_tag_type\n{\n");
    
    for (u32 b = 0; b < TypeTable.MetaTagBucketsCount; b++)
    {
        meta_tag_hash_bucket Bucket = TypeTable.MetaTags[b];
        for (u32 i = 0; i < META_TAG_BUCKET_MAX; i++)
        {
            if (Bucket.Keys[i] != 0)
            {
                string MetaTagIdentifier = Bucket.Values[i].Identifier;
                WriteF(&Generator->MetaTagString, "{ \"%S\", %d },\n", MetaTagIdentifier, MetaTagIdentifier.Length);
                WriteF(&Generator->MetaTagEnum, "MetaTag_%S,\n", MetaTagIdentifier);
            }
        }
    }
    
    WriteF(&Generator->MetaTagString, "};\n");
    WriteF(&Generator->MetaTagEnum, "};\n");
}

#define GS_META_TYPEINFO_GENERATOR_H
#endif // GS_META_TYPEINFO_GENERATOR_H
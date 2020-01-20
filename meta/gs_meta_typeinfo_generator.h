//
// File: gs_meta_typeinfo_generator.h
// Author: Peter Slattery
// Creation Date: 2020-01-19
//
#ifndef GS_META_TYPEINFO_GENERATOR_H

#include <gs_language.h>
#include <gs_string.h>
#include <gs_string_builder.h>

struct typeinfo_generator
{
    string_builder TypeList;
    string_builder StructMembers;
    string_builder TypeDefinitions;
    
    u32 GeneratedInfoTypesCount;
    u32 TypesMax;
    b8* TypesGeneratedMask;
};

internal typeinfo_generator
InitTypeInfoGenerator(type_table TypeTable)
{
    typeinfo_generator Result = {};
    
    Result.TypesMax = TypeTable.Types.Used;
    Result.TypesGeneratedMask = (b8*)malloc(sizeof(b8) * Result.TypesMax);
    GSZeroMemory((u8*)Result.TypesGeneratedMask, Result.TypesMax);
    
    WriteF(&Result.TypeList, "enum gsm_struct_type\n{\n");
    
    WriteF(&Result.TypeDefinitions, "static gsm_struct_type_info StructTypes[] = {\n");
    return Result;
}

internal void
FinishGeneratingTypes(typeinfo_generator* Generator)
{
    WriteF(&Generator->TypeList, "gsm_StructTypeCount,\n};\n\n");
    
    WriteF(&Generator->StructMembers, "\n");
    
    WriteF(&Generator->TypeDefinitions, "};\n");
    WriteF(&Generator->TypeDefinitions, "gsm_u32 StructTypesCount = %d;\n", Generator->GeneratedInfoTypesCount);
}

internal void
GenerateMetaTagInfo (gs_bucket<meta_tag> Tags, string_builder* Builder)
{
    WriteF(Builder, "{");
    for (u32 t = 0; t < Tags.Used; t++)
    {
        meta_tag* Tag = Tags.GetElementAtIndex(t);
        WriteF(Builder, "{ \"%S\", %d }", Tag->Identifier, Tag->Identifier.Length);
        if ((t + 1) < Tags.Used)
        {
            WriteF(Builder, ", ");
        }
    }
    WriteF(Builder, "}, %d", Tags.Used);
}

internal void
GenerateStructMemberInfo (variable_decl* Member, string StructIdentifier, type_table TypeTable, typeinfo_generator* Gen)
{
    WriteF(&Gen->StructMembers, "{ \"%S\", %d, ", Member->Identifier, Member->Identifier.Length);
    WriteF(&Gen->StructMembers, "(u64)&((%S*)0)->%S ", StructIdentifier, Member->Identifier); 
    WriteF(&Gen->StructMembers, "},\n");
}

internal void
GenerateTypeInfo (type_definition* Type, u32 TypeIndex, type_table TypeTable, typeinfo_generator* Generator)
{
    Generator->TypesGeneratedMask[TypeIndex] = true;
    Generator->GeneratedInfoTypesCount++;
    
    {
        // NOTE(Peter): This block MUST come before generating
        // type info for any member types. If it doesn't, it will screw
        // up array ordering
        
        // Lookup Enum
        WriteF(&Generator->TypeList, "gsm_StructType_%S,\n", Type->Identifier);
        
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
            type_definition* MemberType = TypeTable.Types.GetElementAtIndex(Member->TypeIndex);
            
            if (MemberType->Identifier.Length == 0) { continue; } // Don't gen info for anonymous struct and union members
            if (Generator->TypesGeneratedMask[Member->TypeIndex]) { continue; }
            
            GenerateTypeInfo(MemberType, Member->TypeIndex, TypeTable, Generator);
        }
        
        //
        WriteF(&Generator->StructMembers, "static gsm_struct_member_type_info StructMembers_%S[] = {\n", Type->Identifier);
        for (u32 m = 0; m < Type->Struct.MemberDecls.Used; m++)
        {
            variable_decl* Member = Type->Struct.MemberDecls.GetElementAtIndex(m);
            type_definition* MemberType = TypeTable.Types.GetElementAtIndex(Member->TypeIndex);
            
            if (MemberType->Identifier.Length > 0)
            {
                GenerateStructMemberInfo(Member, Type->Identifier, TypeTable, Generator);
            }
            else if (MemberType->Type == TypeDef_Struct ||
                     MemberType->Type == TypeDef_Union)
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
    for (u32 i = 0; i < TypeTable.Types.Used; i++)
    {
        if (Generator->TypesGeneratedMask[i])
        {
            continue;
        }
        
        type_definition* Type = TypeTable.Types.GetElementAtIndex(i);
        if (HasTag(MetaTagFilter, Type->MetaTags))
        {
            GenerateTypeInfo(Type, i, TypeTable, Generator);
        }
    }
}



#define GS_META_TYPEINFO_GENERATOR_H
#endif // GS_META_TYPEINFO_GENERATOR_H
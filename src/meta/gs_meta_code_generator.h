//
// File: gs_meta_code_generator.h
// Author: Peter Slattery
// Creation Date: 2020-01-20
//
#ifndef GS_META_CODE_GENERATOR_H

#include "../gs_libs/gs_string.h"
#include "../gs_libs/gs_string_builder.h"

enum gsm_code_gen_type
{
    gsm_CodeGen_Enum,
    gsm_CodeGen_Count,
};

struct gsm_enum_generator
{
    string Identifier;
    string Prefix;
    b32 EndsWithCount;
};

struct gsm_code_generator
{
    gsm_code_gen_type Type;
    string_builder* Builder;
    
    union
    {
        gsm_enum_generator Enum;
    };
};

// ---------------
//     Enum
// ---------------

internal gsm_code_generator
BeginEnumGeneration(string EnumIdentifier, string ValuePrefix, b32 StartsWithInvalid, b32 EndsWithCount)
{
    gsm_code_generator Gen = {};
    
    // TODO(Peter): TEMP!!
    Gen.Builder = (string_builder*)malloc(sizeof(string_builder));
    *Gen.Builder = {};
    
    Gen.Type = gsm_CodeGen_Enum;
    Gen.Enum.Identifier = EnumIdentifier;
    Gen.Enum.Prefix = ValuePrefix;
    Gen.Enum.EndsWithCount = EndsWithCount;
    
    WriteF(Gen.Builder, "enum %S\n{\n", EnumIdentifier);
    if (StartsWithInvalid)
    {
        WriteF(Gen.Builder, "    %S_Invalid,\n", ValuePrefix);
    }
    
    return Gen;
}

internal gsm_code_generator
BeginEnumGeneration(char* EnumIdentifier, char* ValuePrefix, b32 StartsWithInvalid, b32 EndsWithCount)
{
    return BeginEnumGeneration(MakeStringLiteral(EnumIdentifier),
                               MakeStringLiteral(ValuePrefix),
                               StartsWithInvalid,
                               EndsWithCount);
}

internal void
AddEnumElement(gsm_code_generator* Gen, string ElementIdentifier)
{
    // TODO(Peter): Support const expr defining enum values
    Assert(Gen->Type == gsm_CodeGen_Enum);
    WriteF(Gen->Builder, "    %S_%S,\n", Gen->Enum.Prefix, ElementIdentifier);
}

internal void
AddEnumElement(gsm_code_generator* Gen, char* ElementIdentifier)
{
    AddEnumElement(Gen, MakeStringLiteral(ElementIdentifier));
}

internal void
FinishEnumGeneration(gsm_code_generator* Gen)
{
    Assert(Gen->Type == gsm_CodeGen_Enum);
    
    if (Gen->Enum.EndsWithCount)
    {
        WriteF(Gen->Builder, "    %S_Count,\n", Gen->Enum.Prefix);
    }
    
    WriteF(Gen->Builder, "};\n\n");
}


#define GS_META_CODE_GENERATOR_H
#endif // GS_META_CODE_GENERATOR_H
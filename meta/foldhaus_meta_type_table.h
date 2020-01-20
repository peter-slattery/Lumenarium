//
// File: foldhaus_meta_type_table.h
// Author: Peter Slattery
// Creation Date: 2020-01-17
//
#ifndef FOLDHAUS_META_TYPE_TABLE_H

enum type_definition_type
{
    TypeDef_Invalid,
    
    // NOTE(Peter): tokens with this type require fixup later
    TypeDef_Unknown, 
    TypeDef_Enum,
    TypeDef_Struct,
    TypeDef_Union,
    TypeDef_BasicType,
    TypeDef_FunctionPointer,
    
    TypeDef_Count,
};

struct meta_tag
{
    string Identifier;
};

struct variable_decl
{
    // NOTE(Peter): Because of the way the tokenizer works, we don't lex and parse 
    // at the same time. This means that not all types will be able to be matched
    // up on the first pass through. A TypeIndex of -1 means we need to fixup that
    // type at a later time
    s32 TypeIndex;
    string Identifier;
    b32 Pointer;
    
    // NOTE(Peter): Zero means its not an array, since you cannot initialize a static
    // array to size 0. ie this is invalid: r32 x[0]; and will throw a compiler error
    u32 ArrayCount;
    
    // :SmallAllocationsAllOver
    gs_bucket<meta_tag> MetaTags;
};

struct struct_decl
{
    // TODO(Peter): Lots of tiny arrays everywhere! Pull these into a central allocation
    // buffer somewhere
    // :SmallAllocationsAllOver
    gs_bucket<variable_decl> MemberDecls;
};

struct function_pointer_decl
{
    s32 ReturnTypeIndex;
    // :SmallAllocationsAllOver
    gs_bucket<variable_decl> Parameters;
};

struct enum_decl
{
    gs_bucket<string> Identifiers;
    
    // TODO(Peter): I suppose there are ways to make an enum a 64 bit number
    // for values. Probably need to handle that at some point
    gs_bucket<u32> Values;
};

struct type_definition
{
    string Identifier;
    
    s32 Size;
    gs_bucket<meta_tag> MetaTags;
    
    type_definition_type Type;
    union
    {
        enum_decl Enum;
        struct_decl Struct;
        function_pointer_decl FunctionPtr;
    };
    b32 Pointer;
};

struct type_table
{
    gs_bucket<type_definition> Types;
};

internal b32
HasTag(string Needle, gs_bucket<meta_tag> Tags)
{
    b32 Result = false;
    for (u32 i = 0; i < Tags.Used; i++)
    {
        meta_tag* Tag = Tags.GetElementAtIndex(i);
        if (StringsEqual(Tag->Identifier, Needle))
        {
            Result = true;
        }
    }
    return Result;
}

internal void
CopyMetaTagsAndClear(gs_bucket<token>* Source, gs_bucket<meta_tag>* Dest)
{
    for (u32 i = 0; i < Source->Used; i++)
    {
        token* TagToken = Source->GetElementAtIndex(i);
        
        meta_tag TagDest = {0};
        TagDest.Identifier = TagToken->Text;
        
        Dest->PushElementOnBucket(TagDest);
    }
    Source->Used = 0;
}

internal s32
PushUndeclaredType (string Identifier, type_table* TypeTable)
{
    type_definition UndeclaredTypeDef = {};
    UndeclaredTypeDef.Identifier = Identifier;
    UndeclaredTypeDef.Type = TypeDef_Unknown;
    s32 TypeIndex = (s32)TypeTable->Types.PushElementOnBucket(UndeclaredTypeDef);
    return TypeIndex;
}

internal s32 
GetIndexOfType (string Identifier, type_table TypeTable)
{
    s32 Result = -1;
    for (u32 i = 0; i < TypeTable.Types.Used; i++)
    {
        type_definition* TypeDef = TypeTable.Types.GetElementAtIndex(i);
        if (StringsEqual(Identifier, TypeDef->Identifier))
        {
            Result = i;
            break;
        }
    }
    return Result;
}

internal s32
PushTypeDefOnTypeTable(type_definition TypeDef, type_table* TypeTable)
{
    s32 Index = -1;
    
    s32 ExistingUndeclaredTypeIndex = GetIndexOfType(TypeDef.Identifier, *TypeTable);
    
    // NOTE(Peter): If the identifier length is zero, they will all match with the 
    // first anonymous struct/union member. So every anon struct/union gets its own
    // typeef
    if (ExistingUndeclaredTypeIndex < 0 || TypeDef.Identifier.Length == 0)
    {
        Index = TypeTable->Types.PushElementOnBucket(TypeDef);
    }
    else
    {
        Index = ExistingUndeclaredTypeIndex;
        type_definition* ExistingTypeDef = TypeTable->Types.GetElementAtIndex(ExistingUndeclaredTypeIndex);
        *ExistingTypeDef = TypeDef;
    }
    
    return Index;
}

internal s32
GetSizeOfType (s32 TypeIndex, type_table TypeTable)
{
    s32 Result = -1;
    Assert(TypeIndex >= 0 && (u32)TypeIndex < TypeTable.Types.Used);
    type_definition* TypeDef = TypeTable.Types.GetElementAtIndex(TypeIndex);
    Result = TypeDef->Size;
    return Result;
}

internal s32
GetSizeOfType (string Identifier, type_table TypeTable)
{
    s32 Result = -1;
    for (u32 i = 0; i < TypeTable.Types.Used; i++)
    {
        type_definition* TypeDef = TypeTable.Types.GetElementAtIndex(i);
        if (StringsEqual(Identifier, TypeDef->Identifier))
        {
            Result = TypeDef->Size;
            break;
        }
    }
    return Result;
}

internal b32
VariableDeclsEqual (variable_decl A, variable_decl B)
{
    b32 Result = false;
    if (A.TypeIndex == B.TypeIndex &&
        A.ArrayCount == B.ArrayCount &&
        StringsEqual(A.Identifier, B.Identifier))
    {
        Result = true;
    }
    return Result;
}

internal b32
StructOrUnionsEqual (type_definition A, type_definition B)
{
    // NOTE(Peter): Fairly certain the only places this is used are when we
    // already know the identifiers match
    Assert(StringsEqual(A.Identifier, B.Identifier));
    Assert(A.Type == TypeDef_Struct || A.Type == TypeDef_Union);
    Assert(B.Type == TypeDef_Struct || B.Type == TypeDef_Union);
    
    b32 Result = false;
    if (A.Struct.MemberDecls.Used == B.Struct.MemberDecls.Used)
    {
        Result = true;
        for (u32 i = 0; i < A.Struct.MemberDecls.Used; i++)
        {
            variable_decl* AMember = A.Struct.MemberDecls.GetElementAtIndex(i);
            variable_decl* BMember = A.Struct.MemberDecls.GetElementAtIndex(i);
            
            if (!VariableDeclsEqual(*AMember, *BMember))
            {
                Result = false;
                break;
            }
        }
    }
    return Result;
}

internal s32
FindIndexOfMatchingType (type_definition Match, type_table TypeTable)
{
    s32 Result = -1;
    for (u32 i = 0; i < TypeTable.Types.Used; i++)
    {
        type_definition* TypeDef = TypeTable.Types.GetElementAtIndex(i);
        if (StringsEqual(Match.Identifier, TypeDef->Identifier))
        {
            if (Match.Type == TypeDef_Struct ||
                Match.Type == TypeDef_Union)
            {
                if (StructOrUnionsEqual(Match, *TypeDef))
                {
                    Result = (s32)i;
                    break;
                }
            }
            else
            {
                Result = (s32)i;
                break;
            }
        }
    }
    return Result;
}

internal void FixUpStructSize (s32 StructIndex, type_table TypeTable, errors* Errors);
internal void FixUpUnionSize (s32 UnionIndex, type_table TypeTable, errors* Errors);

internal void
FixupMemberType (variable_decl* Member, type_table TypeTable)
{
    if (Member->TypeIndex == -1)
    {
        Member->TypeIndex = GetIndexOfType(Member->Identifier, TypeTable);
    }
    Assert(Member->TypeIndex >= 0);
}

internal s32
CalculateStructMemberSize (variable_decl Member, type_definition MemberType)
{
    Assert(Member.TypeIndex >= 0);
    // NOTE(Peter): At one point we were Asserting on struct sizes of zero, but
    // that's actually incorrect. It is valid to have an empty struct.
    
    s32 Result = MemberType.Size;
    if (Member.ArrayCount > 0)
    {
        Result *= Member.ArrayCount;
    }
    
    if (Member.Pointer)
    {
        Result = sizeof(void*);
    }
    
    return Result;
}

internal void
FixupStructMember (variable_decl* Member, type_definition* MemberTypeDef, type_table TypeTable, errors* Errors)
{
    // NOTE(Peter): There are a lot of cases where struct members which are pointers
    // to other structs cause interesting behavior here. 
    // For example:
    //     struct foo { foo* Next; }
    // could cause infinite loops if we try and fixup all structs with a size of 0
    // which would happen in this case, because we wouldn't have parsed foo's size 
    // yet, but would begin fixing up foo because of the type of Next
    // Another example:
    //     typedef struct bar bar;
    //     struct foo { bar* Bar; }
    //     struct bar { foo* Foo; }
    // causes the exact same problem, but we cant detect it by just excluding 
    // fixing up StructIndex recursively. 
    // 
    // TL;DR
    // The solution I've chosen to go with is just exclude all pointer members from 
    // causing recursive fixups. Those types should be fixed up at some point in the
    // process, and we already know how big a pointer is in memory, no matter the type
    if (!Member->Pointer)
    {
        if (MemberTypeDef->Size == 0)
        {
            if (MemberTypeDef->Type == TypeDef_Struct)
            {
                FixUpStructSize(Member->TypeIndex, TypeTable, Errors);
            }
            else if (MemberTypeDef->Type == TypeDef_Union)
            {
                FixUpUnionSize(Member->TypeIndex, TypeTable, Errors);
            }
            else 
            {
                if (MemberTypeDef->Type == TypeDef_Unknown)
                {
                    PushFError(Errors, "Error: TypeDef Unknown: %S", MemberTypeDef->Identifier);
                }
                else
                {
                    // TODO(Peter): We don't parse all types yet, so for now, this is just an alert,
                    // not an assert;
#if 0
                    InvalidCodePath;
#else
                    PushFError(Errors, "Error: TypeDef Size = 0. %S\n", MemberTypeDef->Identifier);
#endif
                }
            }
        }
    }
}

internal void
FixUpStructSize (s32 StructIndex, type_table TypeTable, errors* Errors)
{
    type_definition* Struct = TypeTable.Types.GetElementAtIndex(StructIndex);
    Assert(Struct->Type == TypeDef_Struct);
    
    if (HasTag(MakeStringLiteral("breakpoint"), Struct->MetaTags))
    {
        __debugbreak();
    }
    
    s32 SizeAcc = 0;
    for (u32 j = 0; j < Struct->Struct.MemberDecls.Used; j++)
    {
        variable_decl* Member = Struct->Struct.MemberDecls.GetElementAtIndex(j);
        FixupMemberType(Member, TypeTable);
        
        if (Member->TypeIndex >= 0)
        {
            type_definition* MemberTypeDef = TypeTable.Types.GetElementAtIndex(Member->TypeIndex);
            
            FixupStructMember(Member, MemberTypeDef, TypeTable, Errors);
            
            u32 MemberSize = CalculateStructMemberSize(*Member, *MemberTypeDef);
            SizeAcc += MemberSize;
        }
    }
    
    Struct->Size = SizeAcc;
    
    // NOTE(Peter): It is valid to have an empty struct, which would have a size of
    // zero, hence the check here.
    if (Struct->Size == 0 && Struct->Struct.MemberDecls.Used > 0)
    {
        // NOTE(Peter): Because its recursive (it makes sure all type sizes become known
        // if it needs them) we should never get to the end of this function and not have
        // the ability to tell how big something is. 
        // TODO(Peter): We don't parse all types yet however, so for now, this is just an alert,
        // not an assert;
#if 0
        Assert(Struct->Size != 0);
#else
        PushFError(Errors, "Struct Error: Struct Size = 0. %S\n", Struct->Identifier);
#endif
    }
}

internal void
FixUpUnionSize (s32 UnionIndex, type_table TypeTable, errors* Errors)
{
    type_definition* Union = TypeTable.Types.GetElementAtIndex(UnionIndex);
    Assert(Union->Type == TypeDef_Union);
    
    s32 BiggestMemberSize = 0;
    for (u32 j = 0; j < Union->Struct.MemberDecls.Used; j++)
    {
        variable_decl* Member = Union->Struct.MemberDecls.GetElementAtIndex(j);
        FixupMemberType(Member, TypeTable);
        
        if (Member->TypeIndex >= 0)
        {
            type_definition* MemberTypeDef = TypeTable.Types.GetElementAtIndex(Member->TypeIndex);
            FixupStructMember(Member, MemberTypeDef, TypeTable, Errors);
            s32 MemberSize = CalculateStructMemberSize(*Member, *MemberTypeDef);
            BiggestMemberSize = GSMax(BiggestMemberSize, MemberSize);
        }
    }
    
    Union->Size = BiggestMemberSize;
    
    // NOTE(Peter): It is valid to have an empty struct, which would have a size of
    // zero, hence the check here.
    if (Union->Size == 0 && Union->Struct.MemberDecls.Used > 0)
    {
        // NOTE(Peter): Because its recursive (it makes sure all type sizes become known
        // if it needs them) we should never get to the end of this function and not have
        // the ability to tell how big something is
        // TODO(Peter): We don't parse all types yet however, so for now, this is just an alert,
        // not an assert;
#if 0
        Assert(Union->Size != 0);
#else
        PushFError(Errors, "Union Error: Struct Size = 0. %S\n", Union->Identifier);
#endif
    }
}

type_definition CPPBasicTypes[] = {
    { MakeStringLiteral("float"), sizeof(float), {}, TypeDef_BasicType, {}, false },
    { MakeStringLiteral("double"), sizeof(double), {}, TypeDef_BasicType, {}, false },
    { MakeStringLiteral("char"), sizeof(char), {}, TypeDef_BasicType, {}, false },
    { MakeStringLiteral("wchar_t"), sizeof(wchar_t), {}, TypeDef_BasicType, {}, false },
    { MakeStringLiteral("int"), sizeof(int), {}, TypeDef_BasicType, {}, false },
    { MakeStringLiteral("short"), sizeof(short), {}, TypeDef_BasicType, {}, false },
    { MakeStringLiteral("short int"), sizeof(short int), {}, TypeDef_BasicType, {}, false },
    { MakeStringLiteral("long int"), sizeof(long int), {}, TypeDef_BasicType, {}, false },
    { MakeStringLiteral("long long int"), sizeof(long long int), {}, TypeDef_BasicType, {}, false },
    { MakeStringLiteral("void"), sizeof(void*), {}, TypeDef_BasicType, {}, false },
    { MakeStringLiteral("bool"), sizeof(bool), {}, TypeDef_BasicType, {}, false },
    
    // These are from system headers that we aren't parsing atm
    { MakeStringLiteral("HANDLE"), sizeof(void*), {}, TypeDef_BasicType, {}, false },
};

internal void
PopulateTableWithDefaultCPPTypes(type_table* TypeTable)
{
    for (u32 i = 0; i < GSArrayLength(CPPBasicTypes); i++)
    {
        PushTypeDefOnTypeTable(CPPBasicTypes[i], TypeTable);
    }
}

#define FOLDHAUS_META_TYPE_TABLE_H
#endif // FOLDHAUS_META_TYPE_TABLE_H
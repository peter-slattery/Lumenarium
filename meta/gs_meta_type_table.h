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
    TypeDef_Function,
    
    TypeDef_Count,
};

char* TypeDefinitionTypeStrings[] = {
    "TypeDef_Invalid",
    "TypeDef_Unknown",
    "TypeDef_Enum",
    "TypeDef_Struct",
    "TypeDef_Union",
    "TypeDef_BasicType",
    "TypeDef_FunctionPointer",
    "TypeDef_Function",
    "TypeDef_Count",
};


#define InvalidTypeTableHandle type_table_handle{0, 0}
struct type_table_handle
{
    s32 BucketIndex;
    u32 IndexInBucket;
};

// #define TypeHandleIsValid(handle) (!((handle).BucketIndex == 0) && ((handle).IndexInBucket == 0))
inline b32 TypeHandleIsValid(type_table_handle A)
{
    b32 FirstBucket = (A.BucketIndex == 0);
    b32 FirstIndex = (A.IndexInBucket == 0);
    b32 Both = FirstBucket && FirstIndex;
    return !Both;
}

#define TypeHandlesEqual(a, b) (((a).BucketIndex == (b).BucketIndex) && ((a).IndexInBucket == (b).IndexInBucket))

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
    type_table_handle TypeHandle;
    string Identifier;
    b32 Pointer;
    
    // NOTE(Peter): Zero means its not an array, since you cannot initialize a static
    // array to size 0. ie this is invalid: r32 x[0]; and will throw a compiler error
    u32 ArrayCount;
    
    // :SmallAllocationsAllOver
    gs_bucket<type_table_handle> MetaTags;
};

struct struct_decl
{
    b32 IsAnonymous;
    // TODO(Peter): Lots of tiny arrays everywhere! Pull these into a central allocation
    // buffer somewhere
    // :SmallAllocationsAllOver
    gs_bucket<variable_decl> MemberDecls;
};

struct function_pointer_decl
{
    type_table_handle ReturnTypeHandle;
    // :SmallAllocationsAllOver
    gs_bucket<variable_decl> Parameters;
};

struct function_decl
{
    type_table_handle ReturnTypeHandle;
    gs_bucket<variable_decl> Parameters;
    // TODO(Peter): AST?
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
    gs_bucket<type_table_handle> MetaTags;
    
    type_definition_type Type;
    union
    {
        enum_decl Enum;
        struct_decl Struct;
        function_pointer_decl FunctionPtr;
        function_decl Function;
    };
    b32 Pointer;
};

#define TYPE_TABLE_BUCKET_MAX 1024
struct type_table_hash_bucket
{
    u32* Keys;
    type_definition* Values;
};

#define META_TAG_BUCKET_MAX 1024
struct meta_tag_hash_bucket
{
    u32* Keys;
    meta_tag* Values;
};

struct type_table
{
    type_table_hash_bucket* Types;
    u32 TypeBucketsCount;
    
    meta_tag_hash_bucket* MetaTags;
    u32 MetaTagBucketsCount;
};

internal b32
HandlesAreEqual(type_table_handle A, type_table_handle B)
{
    b32 Result = ((A.BucketIndex == B.BucketIndex) && (A.IndexInBucket == B.IndexInBucket));
    return Result;
}

internal u32
HashIdentifier(string Identifier)
{
    u32 IdentHash = HashString(Identifier);
    if (IdentHash == 0)
    {
        // NOTE(Peter): We are excluding a has of zero so taht 
        // the type_table_handle where BucketIndex and IndexInBucket
        // are both zero is an invalid handle
        IdentHash += 1;
    }
    return IdentHash;
}

internal type_table_handle
GetTypeHandle (string Identifier, type_table TypeTable)
{
    type_table_handle Result = InvalidTypeTableHandle;
    
    u32 IdentHash = HashIdentifier(Identifier);
    u32 Index = IdentHash % TYPE_TABLE_BUCKET_MAX;
    
    for (u32 b = 0; b < TypeTable.TypeBucketsCount; b++)
    {
        type_table_hash_bucket Bucket = TypeTable.Types[b];
        if (Bucket.Keys[Index] == IdentHash)
        {
            Result.BucketIndex = b;
            Result.IndexInBucket = Index;
            break;
        }
    }
    
    return Result;
}

internal type_table_handle
GetMetaTagHandle(string Identifier, type_table TypeTable)
{
    type_table_handle Result = InvalidTypeTableHandle;
    
    u32 IdentHash = HashIdentifier(Identifier);
    u32 Index = IdentHash % TYPE_TABLE_BUCKET_MAX;
    
    for (u32 b = 0; b < TypeTable.MetaTagBucketsCount; b++)
    {
        meta_tag_hash_bucket Bucket = TypeTable.MetaTags[b];
        if (Bucket.Keys[Index] == IdentHash)
        {
            Result.BucketIndex = b;
            Result.IndexInBucket = Index;
            break;
        }
    }
    
    return Result;
}

internal type_table_handle
GetMetaTagHandleWithIdentifier(string Identifier, type_table TypeTable)
{
    type_table_handle Result = InvalidTypeTableHandle;
    
    u32 IdentHash = HashIdentifier(Identifier);
    u32 Index = IdentHash % META_TAG_BUCKET_MAX;
    for (u32 b = 0; b < TypeTable.MetaTagBucketsCount; b++)
    {
        meta_tag_hash_bucket* Bucket = TypeTable.MetaTags + b;
        if (Bucket->Keys[Index] == IdentHash)
        {
            Result.BucketIndex = b;
            Result.IndexInBucket = Index;
            break;
        }
    }
    
    return Result;
}

internal b32
HasTag(string Needle, gs_bucket<type_table_handle> Tags, type_table TypeTable)
{
    b32 Result = false;
    type_table_handle NeedleTagHandle = GetMetaTagHandleWithIdentifier(Needle, TypeTable);
    
    if (TypeHandleIsValid(NeedleTagHandle))
    {
        for (u32 i = 0; i < Tags.Used; i++)
        {
            type_table_handle* TagHandle = Tags.GetElementAtIndex(i);
            if (HandlesAreEqual(*TagHandle, NeedleTagHandle))
            {
                Result = true;
                break;
            }
        }
    }
    
    return Result;
}

internal void
CopyMetaTagsAndClear(gs_bucket<type_table_handle>* Source, gs_bucket<type_table_handle>* Dest)
{
    for (u32 i = 0; i < Source->Used; i++)
    {
        type_table_handle* TagToken = Source->GetElementAtIndex(i);
        Dest->PushElementOnBucket(*TagToken);
    }
    Source->Used = 0;
}

internal type_table_handle
FindSlotForTypeIdentifier(u32 IdentHash, type_table* TypeTable)
{
    type_table_handle Result = InvalidTypeTableHandle;
    u32 Index = IdentHash % TYPE_TABLE_BUCKET_MAX;
    
    for (u32 b = 0; b < TypeTable->TypeBucketsCount; b++)
    {
        type_table_hash_bucket* Bucket = TypeTable->Types + b;
        if (Bucket->Keys[Index] == 0)
        {
            Result.BucketIndex = b;
            Result.IndexInBucket = Index;
            break;
        }
    }
    
    if (!TypeHandleIsValid(Result))
    {
        // Grow Hash Table
        u32 NewTypeBucketIndex = TypeTable->TypeBucketsCount++;
        u32 NewTypesSize = TypeTable->TypeBucketsCount * sizeof(type_table_hash_bucket);
        TypeTable->Types = (type_table_hash_bucket*)realloc(TypeTable->Types, NewTypesSize);
        
        type_table_hash_bucket* NewBucket = TypeTable->Types + NewTypeBucketIndex;
        NewBucket->Keys = (u32*)malloc(sizeof(u32) * TYPE_TABLE_BUCKET_MAX);
        NewBucket->Values = (type_definition*)malloc(sizeof(type_definition) * TYPE_TABLE_BUCKET_MAX);
        GSZeroMemory((u8*)NewBucket->Keys, sizeof(u32) * TYPE_TABLE_BUCKET_MAX);
        GSZeroMemory((u8*)NewBucket->Values, sizeof(type_definition) * TYPE_TABLE_BUCKET_MAX);
        
        Result.BucketIndex = NewTypeBucketIndex;
        Result.IndexInBucket = Index;
    }
    
    // NOTE(Peter): Because we are growing the hashtable, this should never be an invalid 
    // type handle
    Assert(TypeHandleIsValid(Result));
    return Result;
}

internal type_table_handle
FindSlotForMetaTag(u32 IdentHash, type_table* TypeTable)
{
    type_table_handle Result = InvalidTypeTableHandle;
    u32 Index = IdentHash % TYPE_TABLE_BUCKET_MAX;
    
    for (u32 b = 0; b < TypeTable->MetaTagBucketsCount; b++)
    {
        meta_tag_hash_bucket* Bucket = TypeTable->MetaTags + b;
        if (Bucket->Keys[Index] == 0)
        {
            Result.BucketIndex = b;
            Result.IndexInBucket = Index;
            break;
        }
    }
    
    if (!TypeHandleIsValid(Result))
    {
        u32 NewMetaBucketIndex = TypeTable->MetaTagBucketsCount++;
        u32 NewMetaBucketListSize = TypeTable->MetaTagBucketsCount * sizeof(meta_tag_hash_bucket);
        TypeTable->MetaTags = (meta_tag_hash_bucket*)realloc(TypeTable->MetaTags, NewMetaBucketListSize);
        
        meta_tag_hash_bucket* NewBucket = TypeTable->MetaTags + NewMetaBucketIndex;
        NewBucket->Keys = (u32*)malloc(sizeof(u32) * TYPE_TABLE_BUCKET_MAX);
        NewBucket->Values = (meta_tag*)malloc(sizeof(meta_tag) * TYPE_TABLE_BUCKET_MAX);
        GSZeroMemory((u8*)NewBucket->Keys, sizeof(u32) * TYPE_TABLE_BUCKET_MAX);
        GSZeroMemory((u8*)NewBucket->Values, sizeof(meta_tag) * TYPE_TABLE_BUCKET_MAX);
        
        Result.BucketIndex = NewMetaBucketIndex;
        Result.IndexInBucket = Index;
    }
    
    return Result;
}

internal type_table_handle
PushTypeOnHashTable(type_definition TypeDef, type_table* TypeTable)
{
    u32 IdentHash = HashIdentifier(TypeDef.Identifier);
    type_table_handle Result = FindSlotForTypeIdentifier(IdentHash, TypeTable);
    
    type_table_hash_bucket* Bucket = TypeTable->Types + Result.BucketIndex;
    Bucket->Keys[Result.IndexInBucket] = IdentHash;
    Bucket->Values[Result.IndexInBucket] = TypeDef;
    
#if PRINT_DIAGNOSTIC_INFO
    printf("Registering Type\n");
    printf("    %.*s\n", StringExpand(TypeDef.Identifier));
    printf("    Type: %s\n\n", TypeDefinitionTypeStrings[TypeDef.Type]);
#endif
    
    return Result;
}

internal type_table_handle
PushUndeclaredType (string Identifier, type_table* TypeTable)
{
    type_definition UndeclaredTypeDef = {};
    UndeclaredTypeDef.Identifier = Identifier;
    UndeclaredTypeDef.Type = TypeDef_Unknown;
    type_table_handle Result = PushTypeOnHashTable(UndeclaredTypeDef, TypeTable);
    return Result;
}

internal type_table_handle
PushMetaTagOnTable(meta_tag Tag, type_table* TypeTable)
{
    u32 TagIdentifierHash = HashIdentifier(Tag.Identifier);
    type_table_handle Result = FindSlotForMetaTag(TagIdentifierHash, TypeTable);
    
    meta_tag_hash_bucket* Bucket = TypeTable->MetaTags + Result.BucketIndex;
    Bucket->Keys[Result.IndexInBucket] = TagIdentifierHash;
    Bucket->Values[Result.IndexInBucket] = Tag;
    
#if PRINT_DIAGNOSTIC_INFO
    printf("Registering Meta Tag\n");
    printf("    %.*s\n\n", StringExpand(Tag.Identifier));
#endif
    
    return Result;
}

// Guaranteed to return a valid result
internal type_definition* 
GetTypeDefinition(type_table_handle Handle, type_table TypeTable)
{
    Assert(TypeHandleIsValid(Handle));
    type_definition* Result = 0;
    if (TypeTable.Types[Handle.BucketIndex].Keys != 0)
    {
        Result = TypeTable.Types[Handle.BucketIndex].Values + Handle.IndexInBucket;
    }
    return Result;
}

// May return zero
internal type_definition* 
GetTypeDefinitionUnsafe(type_table_handle Handle, type_table TypeTable)
{
    type_definition* Result = 0;
    if (TypeTable.Types[Handle.BucketIndex].Keys != 0)
    {
        Result = TypeTable.Types[Handle.BucketIndex].Values + Handle.IndexInBucket;
    }
    return Result;
}

internal meta_tag*
GetMetaTag(type_table_handle Handle, type_table TypeTable)
{
    meta_tag* Result = 0;
    if (TypeTable.MetaTags[Handle.BucketIndex].Keys != 0)
    {
        Result = TypeTable.MetaTags[Handle.BucketIndex].Values + Handle.IndexInBucket;
    }
    return Result;
}

internal type_definition* 
GetTypeDefinition(string Identifier, type_table TypeTable)
{
    type_definition* Result = 0;
    u32 IdentHash = HashIdentifier(Identifier);
    u32 Index = IdentHash % TYPE_TABLE_BUCKET_MAX;
    for (u32 b = 0; b < TypeTable.TypeBucketsCount; b++)
    {
        type_table_hash_bucket Bucket = TypeTable.Types[b];
        if (Bucket.Keys[Index] == IdentHash )
        {
            Result = Bucket.Values + Index;
            break;
        }
    }
    return Result;
}

internal type_table_handle
PushTypeDefOnTypeTable(type_definition TypeDef, type_table* TypeTable)
{
    // NOTE(Peter): We don't accept type definitions with empty identifiers.
    // If a struct or union is anonymous, it should be assigned a name of the form
    // parent_struct_name_# where # is the member index
    // ie.
    //    struct foo { int a; union { int x }; };
    //    the union in foo would have the identifier foo_1
    Assert(TypeDef.Identifier.Length != 0);
    
    type_table_handle Result = InvalidTypeTableHandle;
    type_table_handle ExistingUndeclaredTypeHandle = GetTypeHandle(TypeDef.Identifier, *TypeTable);
    
    if (!TypeHandleIsValid(ExistingUndeclaredTypeHandle))
    {
        Result = PushTypeOnHashTable(TypeDef, TypeTable);
    }
    else
    {
        Result = ExistingUndeclaredTypeHandle;
        type_definition* ExistingTypeDef = GetTypeDefinition(Result, *TypeTable);
        Assert(ExistingTypeDef != 0);
        *ExistingTypeDef = TypeDef;
    }
    
    return Result;
}

internal s32
GetSizeOfType (type_table_handle TypeHandle, type_table TypeTable)
{
    s32 Result = -1;
    type_definition* TypeDef = GetTypeDefinition(TypeHandle, TypeTable);
    if (TypeDef)
    {
        Result = TypeDef->Size;
    }
    return Result;
}

internal s32
GetSizeOfType (string Identifier, type_table TypeTable)
{
    s32 Result = -1;
    type_definition* TypeDef = GetTypeDefinition(Identifier, TypeTable);
    if (TypeDef)
    {
        Result = TypeDef->Size;
    }
    return Result;
}

internal b32
VariableDeclsEqual (variable_decl A, variable_decl B)
{
    b32 Result = false;
    if (TypeHandlesEqual(A.TypeHandle, B.TypeHandle) &&
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

internal type_table_handle
FindHandleOfMatchingType (type_definition Match, type_table TypeTable)
{
    type_table_handle Result = InvalidTypeTableHandle;
    type_table_handle Handle = GetTypeHandle(Match.Identifier, TypeTable);
    if (TypeHandleIsValid(Handle))
    {
        Result = Handle;
    }
    return Result;
}

internal void FixUpStructSize (type_table_handle TypeHandle, type_table TypeTable, errors* Errors);
internal void FixUpUnionSize (type_table_handle TypeHandle, type_table TypeTable, errors* Errors);

internal void
FixupMemberType (variable_decl* Member, type_table TypeTable)
{
    if (!TypeHandleIsValid(Member->TypeHandle))
    {
        Member->TypeHandle = GetTypeHandle(Member->Identifier, TypeTable);
    }
    Assert(TypeHandleIsValid(Member->TypeHandle));
}

internal s32
CalculateStructMemberSize (variable_decl Member, type_definition MemberType)
{
    Assert(TypeHandleIsValid(Member.TypeHandle));
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
                FixUpStructSize(Member->TypeHandle, TypeTable, Errors);
            }
            else if (MemberTypeDef->Type == TypeDef_Union)
            {
                FixUpUnionSize(Member->TypeHandle, TypeTable, Errors);
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
FixUpStructSize (type_table_handle TypeHandle, type_table TypeTable, errors* Errors)
{
    type_definition* Struct = GetTypeDefinition(TypeHandle, TypeTable);
    Assert(Struct->Type == TypeDef_Struct);
    
    if (HasTag(MakeStringLiteral("breakpoint"), Struct->MetaTags, TypeTable))
    {
        __debugbreak();
    }
    
    s32 SizeAcc = 0;
    for (u32 j = 0; j < Struct->Struct.MemberDecls.Used; j++)
    {
        variable_decl* Member = Struct->Struct.MemberDecls.GetElementAtIndex(j);
        FixupMemberType(Member, TypeTable);
        
        if (TypeHandleIsValid(Member->TypeHandle))
        {
            type_definition* MemberTypeDef = GetTypeDefinition(Member->TypeHandle, TypeTable);
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
FixUpUnionSize (type_table_handle TypeHandle, type_table TypeTable, errors* Errors)
{
    type_definition* Union = GetTypeDefinition(TypeHandle, TypeTable);
    Assert(Union->Type == TypeDef_Union);
    
    s32 BiggestMemberSize = 0;
    for (u32 j = 0; j < Union->Struct.MemberDecls.Used; j++)
    {
        variable_decl* Member = Union->Struct.MemberDecls.GetElementAtIndex(j);
        FixupMemberType(Member, TypeTable);
        
        if (TypeHandleIsValid(Member->TypeHandle))
        {
            type_definition* MemberTypeDef = GetTypeDefinition(Member->TypeHandle, TypeTable);
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

internal void
PrintTypeDefinition(type_definition TypeDef, type_table TypeTable)
{
    printf("Type: %.*s\n", StringExpand(TypeDef.Identifier));
    printf("    Size: %d\n", TypeDef.Size);
    
    printf("    Meta Tags: ");
    for (u32 m = 0; m < TypeDef.MetaTags.Used; m++)
    {
        type_table_handle TagHandle = *TypeDef.MetaTags.GetElementAtIndex(m);
        meta_tag* Tag = GetMetaTag(TagHandle, TypeTable);
        printf("%.*s ", StringExpand(Tag->Identifier));
    }
    printf("\n");
    
    printf("    Type: %s\n", TypeDefinitionTypeStrings[TypeDef.Type]);
    
    switch(TypeDef.Type)
    {
        case TypeDef_Unknown:
        {
        } break;
        
        case TypeDef_Enum:
        {
        } break;
        
        case TypeDef_Struct:
        {
        } break;
        
        case TypeDef_Union:
        {
        } break;
        
        case TypeDef_BasicType:
        {
        } break;
        
        case TypeDef_FunctionPointer:
        {
        } break;
        
        case TypeDef_Function:
        {
            type_definition* ReturnType = GetTypeDefinition(TypeDef.Function.ReturnTypeHandle, TypeTable);
            printf("    Returns: %.*s\n", StringExpand(ReturnType->Identifier));
            
            printf("    Parameters: ");
            for (u32 p = 0; p < TypeDef.Function.Parameters.Used; p++)
            {
                variable_decl* Param = TypeDef.Function.Parameters.GetElementAtIndex(p);
                type_definition* ParamType = GetTypeDefinition(Param->TypeHandle, TypeTable);
                printf("%.*s %.*s, ", StringExpand(ParamType->Identifier), StringExpand(Param->Identifier));
            }
        } break;
        
        case TypeDef_Invalid:
        case TypeDef_Count:
        {
            printf("???\n");
        } break;
    }
}

internal void
PrintTypeTable(type_table TypeTable)
{
    for (u32 b = 0; b < TypeTable.TypeBucketsCount; b++)
    {
        type_table_hash_bucket Bucket = TypeTable.Types[b];
        for (u32 i = 0; i < TYPE_TABLE_BUCKET_MAX; i++)
        {
            if (Bucket.Keys[i] != 0)
            {
                PrintTypeDefinition(Bucket.Values[i], TypeTable);
            }
        }
    }
}

internal void
DEBUGPrintTypeTableMembership(type_table TypeTable)
{
    printf("\n--- Type Table Membership --\n");
    u32 SlotsAvailable = 0;
    u32 SlotsFilled = 0;
    u32 TotalSlots = TypeTable.TypeBucketsCount * TYPE_TABLE_BUCKET_MAX;
    for (u32 b = 0; b < TypeTable.TypeBucketsCount; b++)
    {
        type_table_hash_bucket Bucket = TypeTable.Types[b];
        for (u32 i = 0; i < TYPE_TABLE_BUCKET_MAX; i++)
        {
            if (Bucket.Keys[i] != 0)
            {
                SlotsFilled++;
                printf("[x] ");
            }
            else
            {
                SlotsAvailable++;
                printf("[ ] ");
            }
        }
        printf("\n");
    }
    
    r32 PercentUsed = (r32)SlotsFilled / (r32)TotalSlots;
    printf("Slots Available: %d\n", SlotsAvailable);
    printf("Slots Filled:    %d\n", SlotsFilled);
    printf("Total Slots:     %d\n", TotalSlots);
    printf("Percent Used:    %f\n", PercentUsed);
    printf("\n");
}

#define FOLDHAUS_META_TYPE_TABLE_H
#endif // FOLDHAUS_META_TYPE_TABLE_H
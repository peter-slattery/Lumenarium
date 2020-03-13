//
// File: foldhaus_meta.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-19
//
#ifndef FOLDHAUS_META_CPP

#include "gs_meta.cpp"
#include "gs_meta_typeinfo_generator.h"

internal void
GenerateNodeMetaInfo (gsm_code_generator* NodeTypeGen, string_builder* NodeSpecificationGen, string_builder* CallNodeProcGen, gs_meta_preprocessor Meta)
{
    // TODO(Peter): Create a FilterTypesByTag function to create a contiguous array
    // of type_definition** 
    
    WriteF(NodeSpecificationGen, "static node_specification_ NodeSpecifications[] = {\n");
    
    WriteF(CallNodeProcGen, "void CallNodeProc(node_type Type, u8* NodeData)\n{\n");
    WriteF(CallNodeProcGen, "    switch(Type) { \n");
    for (u32 b = 0; b < Meta.TypeTable.TypeBucketsCount; b++)
    {
        type_table_hash_bucket Bucket = Meta.TypeTable.Types[b];
        for (u32 i = 0; i < TYPE_TABLE_BUCKET_MAX; i++)
        {
            if (Bucket.Keys[i] == 0) { continue; }
            
            type_definition* Decl = Bucket.Values + i;
            if (HasTag(MakeStringLiteral("node_proc"), Decl->MetaTags, Meta.TypeTable) &&
                Decl->Type == TypeDef_Function)
            {
                if (Decl->Function.Parameters.Used > 1)
                {
                    WriteF(CallNodeProcGen, "ERROR: Procedure tagged with node_proc has more than one parameter\n");
                    continue;
                }
                
                AddEnumElement(NodeTypeGen, Decl->Identifier);
                
                variable_decl* Param = Decl->Function.Parameters.GetElementAtIndex(0);
                type_table_handle ParamTypeHandle = Param->TypeHandle;
                type_definition* ParamType = GetTypeDefinition(ParamTypeHandle, Meta.TypeTable);
                
                type_table_handle ReturnTypeHandle = Decl->Function.ReturnTypeHandle;
                type_definition* ReturnType = GetTypeDefinition(ReturnTypeHandle, Meta.TypeTable);
                
                WriteF(NodeSpecificationGen, "{ NodeType_%S, {\"%S\", %d}, gsm_StructType_%S }, \n",
                       Decl->Identifier,
                       Decl->Identifier,
                       Decl->Identifier.Length,
                       ParamType->Identifier);
                
                WriteF(CallNodeProcGen, "        case NodeType_%.*s:\n", StringExpand(Decl->Identifier));
                WriteF(CallNodeProcGen, "        {\n");
                WriteF(CallNodeProcGen, "            %.*s(", StringExpand(Decl->Identifier));
                WriteF(CallNodeProcGen, "(%.*s*)NodeData", StringExpand(ParamType->Identifier));
                
                WriteF(CallNodeProcGen, ");\n");
                WriteF(CallNodeProcGen, "        } break;\n");
            }
        }
    }
    WriteF(CallNodeProcGen, "    }\n");
    WriteF(CallNodeProcGen, "}\n\n");
    
    WriteF(NodeSpecificationGen, "};\n\n");
    
    FinishEnumGeneration(NodeTypeGen);
}

struct panel_elements
{
    string PanelIdentifier;
    type_table_handle InitProcDecl;
    type_table_handle CleanupProcDecl;
    type_table_handle RenderProcDecl;
    type_table_handle PanelCommandsStruct;
};

internal b32
StringIsPrefixedBy (string Prefix, string TestString)
{
    b32 Result = false;
    
    if (TestString.Length >= Prefix.Length)
    {
        Result = true;
        for (s32 i = 0; i < Prefix.Length; i++)
        {
            if (Prefix.Memory[i] != TestString.Memory[i])
            {
                Result = false;
                break;
            }
        }
    }
    
    return Result;
}

internal void
AttemptPlacePanelProc(type_table_handle ProcHandle, type_table TypeTable, gs_bucket<panel_elements>* Panels)
{
    string InitProcTag = MakeStringLiteral("panel_init");
    string CleanupProcTag = MakeStringLiteral("panel_cleanup");
    string RenderProcTag = MakeStringLiteral("panel_render");
    string PanelTypePrefix = MakeStringLiteral("panel_type_");
    
    type_definition* Decl = GetTypeDefinition(ProcHandle, TypeTable);
    meta_tag* PanelTypeTag = 0;
    
    for (u32 i = 0; i < Decl->MetaTags.Used; i++)
    {
        type_table_handle MetaTagHandle = *Decl->MetaTags.GetElementAtIndex(i);
        meta_tag* MetaTag = GetMetaTag(MetaTagHandle, TypeTable);
        if (StringIsPrefixedBy(PanelTypePrefix, MetaTag->Identifier))
        {
            PanelTypeTag = MetaTag;
            break;
        }
    }
    
    if (PanelTypeTag != 0)
    {
        s32 PanelIndex = -1;
        for (u32 i = 0; i < Panels->Used; i++)
        {
            panel_elements* Panel = Panels->GetElementAtIndex(i);
            if (StringsEqual(Panel->PanelIdentifier, PanelTypeTag->Identifier))
            {
                PanelIndex = (s32)i;
                break;
            }
        }
        if (PanelIndex < 0)
        {
            panel_elements NewPanel = {0};
            NewPanel.PanelIdentifier = PanelTypeTag->Identifier;
            PanelIndex = Panels->PushElementOnBucket(NewPanel);
        }
        
        Assert(PanelIndex >= 0);
        panel_elements* PanelElements = Panels->GetElementAtIndex(PanelIndex);
        if (HasTag(InitProcTag, Decl->MetaTags, TypeTable))
        {
            PanelElements->InitProcDecl = ProcHandle;
        }
        else if (HasTag(CleanupProcTag, Decl->MetaTags, TypeTable))
        {
            PanelElements->CleanupProcDecl = ProcHandle;
        }
        else if (HasTag(RenderProcTag, Decl->MetaTags, TypeTable))
        {
            PanelElements->RenderProcDecl = ProcHandle;
        }
    }
}

internal void
AttemptPlacePanelCommands(type_table_handle StructHandle, type_table TypeTable, gs_bucket<panel_elements>* Panels)
{
    string CommandsTag = MakeStringLiteral("panel_commands");
    
    type_definition* Decl = GetTypeDefinition(StructHandle, TypeTable);
    if (HasTag(CommandsTag, Decl->MetaTags, TypeTable))
    {
        for (u32 i = 0; i < Decl->MetaTags.Used; i++)
        {
            type_table_handle MetaTagHandle = *Decl->MetaTags.GetElementAtIndex(i);
            meta_tag* MetaTag = GetMetaTag(MetaTagHandle, TypeTable);
            printf("%.*s, ", StringExpand(MetaTag->Identifier));
        }
        printf("\n");
    }
}

internal void
MakeReadableIdentifier(string* Identifier)
{
    for (s32 i = 0; i < Identifier->Length; i++)
    {
        char At = Identifier->Memory[i];
        if (At == '_')
        {
            Identifier->Memory[i] = ' ';
        }
        else if (IsAlpha(At) && (i == 0 || IsWhitespace(Identifier->Memory[i - 1])))
        {
            Identifier->Memory[i] = ToUpper(At);
        }
    }
}

internal void
GeneratePanelMetaInfo(gs_meta_preprocessor Meta, string_builder* PanelEnumGen, string_builder* PanelCodeGen)
{
    gs_bucket<panel_elements> Panels = {0};
    
    for (u32 b = 0; b < Meta.TypeTable.TypeBucketsCount; b++)
    {
        type_table_hash_bucket Bucket = Meta.TypeTable.Types[b];
        for (u32 i = 0; i < TYPE_TABLE_BUCKET_MAX; i++)
        {
            if (Bucket.Keys[i] == 0) { continue; }
            
            type_table_handle DeclHandle = {(s32)b, i};
            type_definition* Decl = Bucket.Values + i;
            
            if (Decl->Type == TypeDef_Function)
            {
                AttemptPlacePanelProc(DeclHandle, Meta.TypeTable, &Panels);
            }
            else if (Decl->Type == TypeDef_Struct)
            {
                AttemptPlacePanelCommands(DeclHandle, Meta.TypeTable, &Panels);
            }
        }
    }
    
    WriteF(PanelEnumGen, "enum panel_type {\n");
    WriteF(PanelCodeGen, "global_variable s32 GlobalPanelDefsCount = %d;\n", Panels.Used);
    WriteF(PanelCodeGen, "global_variable panel_definition GlobalPanelDefs[] = {\n");
    for (u32 i = 0; i < Panels.Used; i++)
    {
        panel_elements* Panel = Panels.GetElementAtIndex(i);
        string PanelIdentifier = {0};
        PanelIdentifier.Max = Panel->PanelIdentifier.Length;
        PanelIdentifier.Memory = (char*)malloc(sizeof(char) * PanelIdentifier.Max);
        CopyStringTo(Substring(Panel->PanelIdentifier, 11), &PanelIdentifier);
        MakeReadableIdentifier(&PanelIdentifier);
        
        type_definition* InitDecl = GetTypeDefinition(Panel->InitProcDecl, Meta.TypeTable);
        type_definition* CleanupDecl = GetTypeDefinition(Panel->CleanupProcDecl, Meta.TypeTable);
        type_definition* RenderDecl = GetTypeDefinition(Panel->RenderProcDecl, Meta.TypeTable);
        
        WriteF(PanelCodeGen, "{ \"%S\", %d, ", PanelIdentifier, PanelIdentifier.Length);
        WriteF(PanelCodeGen, "%S, ", InitDecl->Identifier);
        WriteF(PanelCodeGen, "%S, ", CleanupDecl->Identifier);
        WriteF(PanelCodeGen, "%S, ", RenderDecl->Identifier);
        
        // TODO(Peter): This is a shortcut cause I'm being lazy. We arent' putting arrays into the
        // AST when we parse our codebase so there's no way to tag the array of Commands for each
        // panel for use here. Instead, I'm just requiring that the array be of the form
        // <panel_name_base>_Commands where panel_name_base is whatever the Init function is called
        // minus _Input. So for example, if you have ScupltureView_Init, then the panel_name_base is
        // SculptureView and the commands array must be called SculptureView_Commands.
        // Ideally we actually go through and parse these arrays.
        string InitSuffix = MakeStringLiteral("_Init");
        string PanelNameBase = Substring(InitDecl->Identifier, 0, InitDecl->Identifier.Length - InitSuffix.Length);
        WriteF(PanelCodeGen, "%S_Commands, ", PanelNameBase);
        WriteF(PanelCodeGen, "%S_CommandsCount ", PanelNameBase);
        
        WriteF(PanelEnumGen, "PanelType_%S,\n", PanelNameBase);
        
        WriteF(PanelCodeGen, "},\n");
    }
    WriteF(PanelCodeGen, "};\n");
    WriteF(PanelEnumGen, "};\n");
}

internal string
AllocAndConcatStrings(string First, string Second)
{
    string Result = {0};
    Result.Max = First.Length + Second.Length + 1;
    Result.Memory = (char*)malloc(sizeof(char) * Result.Max);
    ConcatString(First, &Result);
    ConcatString(Second, &Result);
    NullTerminate(&Result);
    Result.Length -= 1;
    return Result;
}

int main(int ArgCount, char* Args[])
{
    if (ArgCount <= 1)
    {
        printf("Please supply at least one source directory to analyze.\n");
        return 0;
    }
    
    string RootFile = MakeString(Args[1]);
    s32 IndexOfLastSlash = ReverseSearchForCharInSet(RootFile, "\\/");
    string WorkingDirectory = Substring(RootFile, 0, IndexOfLastSlash + 1);
    string GeneratedDirectoryName = MakeStringLiteral("generated\\");
    string GeneratedFilesDirectory = AllocAndConcatStrings(WorkingDirectory, GeneratedDirectoryName);
    printf("Putting Generated Files In %s\n", GeneratedFilesDirectory.Memory);
    
    gs_meta_preprocessor Meta = PreprocessProgram(Args[1]);
    
    typeinfo_generator TypeGenerator = InitTypeInfoGenerator(Meta.TypeTable);
    GenerateMetaTagList(Meta.TypeTable, &TypeGenerator);
    GenerateFilteredTypeInfo(MakeStringLiteral("node_struct"), Meta.TypeTable, &TypeGenerator);
    GenerateFilteredTypeInfo(MakeStringLiteral("gen_type_info"), Meta.TypeTable, &TypeGenerator);
    FinishGeneratingTypes(&TypeGenerator);
    
    gsm_code_generator NodeTypeGen = BeginEnumGeneration("node_type", "NodeType", false, true);
    string_builder NodeSpecificationGen = {0};
    string_builder CallNodeProcGen = {0};
    GenerateNodeMetaInfo(&NodeTypeGen, &NodeSpecificationGen, &CallNodeProcGen, Meta);
    
    string_builder PanelEnumGen = {0};
    string_builder PanelCodeGen = {0};
    GeneratePanelMetaInfo(Meta, &PanelEnumGen, &PanelCodeGen);
    
    string TypeInfoHFilePath = AllocAndConcatStrings(GeneratedFilesDirectory, MakeStringLiteral("gs_meta_generated_typeinfo.h"));
    FILE* TypeInfoH = fopen(TypeInfoHFilePath.Memory, "w");
    if (TypeInfoH)
    {
        WriteStringBuilderToFile(TypeGenerator.MetaTagEnum, TypeInfoH);
        WriteStringBuilderToFile(TypeGenerator.MetaTagString, TypeInfoH);
        WriteStringBuilderToFile(*TypeGenerator.TypeList.Builder, TypeInfoH);
        WriteStringBuilderToFile(TypeGenerator.StructMembers, TypeInfoH);
        WriteStringBuilderToFile(TypeGenerator.TypeDefinitions, TypeInfoH);
        fclose(TypeInfoH);
    }
    else
    {
        printf("Error: Unable to open file at %.*s\n", StringExpand(TypeInfoHFilePath));
    }
    
    string NodeInfoHFilePath = AllocAndConcatStrings(GeneratedFilesDirectory, MakeStringLiteral("foldhaus_nodes_generated.h"));
    FILE* NodeInfoH = fopen(NodeInfoHFilePath.Memory, "w");
    if (NodeInfoH)
    {
        WriteStringBuilderToFile(*NodeTypeGen.Builder, NodeInfoH);
        WriteStringBuilderToFile(NodeSpecificationGen, NodeInfoH);
        WriteStringBuilderToFile(CallNodeProcGen, NodeInfoH);
        fclose(NodeInfoH);
    }
    else
    {
        printf("Error: Unable to open file at %.*s\n", StringExpand(NodeInfoHFilePath));
    }
    
    string PanelInfoHFilePath = AllocAndConcatStrings(GeneratedFilesDirectory, MakeStringLiteral("foldhaus_panels_generated.h"));
    FILE* PanelInfoH = fopen(PanelInfoHFilePath.Memory, "w");
    if (PanelInfoH)
    {
        WriteStringBuilderToFile(PanelEnumGen, PanelInfoH);
        WriteStringBuilderToFile(PanelCodeGen, PanelInfoH);
        fclose(PanelInfoH);
    }
    else
    {
        printf("Error: Unable to open file at %.*s\n", StringExpand(PanelInfoHFilePath));
    }
    
    FinishMetaprogram(&Meta);
    
    //__debugbreak();
    return 0;
}

#define FOLDHAUS_META_CPP
#endif // FOLDHAUS_META_CPP
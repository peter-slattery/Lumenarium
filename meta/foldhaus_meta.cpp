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
            if (HasTag(MakeStringLiteral("node_proc"), Decl->MetaTags) &&
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

int main(int ArgCount, char* Args[])
{
    if (ArgCount <= 1)
    {
        printf("Please supply at least one source directory to analyze.\n");
        return 0;
    }
    
    gs_meta_preprocessor Meta = PreprocessProgram(Args[1]);
    
    typeinfo_generator TypeGenerator = InitTypeInfoGenerator(Meta.TypeTable);
    GenerateFilteredTypeInfo(MakeStringLiteral("node_struct"), Meta.TypeTable, &TypeGenerator);
    GenerateFilteredTypeInfo(MakeStringLiteral("gen_type_info"), Meta.TypeTable, &TypeGenerator);
    FinishGeneratingTypes(&TypeGenerator);
    
    gsm_code_generator NodeTypeGen = BeginEnumGeneration("node_type", "NodeType", false, true);
    string_builder NodeSpecificationGen = {0};
    string_builder CallNodeProcGen = {0};
    GenerateNodeMetaInfo(&NodeTypeGen, &NodeSpecificationGen, &CallNodeProcGen, Meta);
    
    string_builder PanelInfoGen = {0};
    
    FILE* TypeInfoH = fopen("C:\\projects\\foldhaus\\src\\generated\\gs_meta_generated_typeinfo.h", "w");
    if (TypeInfoH)
    {
        WriteStringBuilderToFile(*TypeGenerator.TypeList.Builder, TypeInfoH);
        WriteStringBuilderToFile(TypeGenerator.StructMembers, TypeInfoH);
        WriteStringBuilderToFile(TypeGenerator.TypeDefinitions, TypeInfoH);
        fclose(TypeInfoH);
    }
    
    FILE* NodeInfoH = fopen("C:\\projects\\foldhaus\\src\\generated\\foldhaus_nodes_generated.h", "w");
    if (NodeInfoH)
    {
        WriteStringBuilderToFile(*NodeTypeGen.Builder, NodeInfoH);
        WriteStringBuilderToFile(NodeSpecificationGen, NodeInfoH);
        WriteStringBuilderToFile(CallNodeProcGen, NodeInfoH);
        fclose(NodeInfoH);
    }
    
    FinishMetaprogram(&Meta);
    
    //__debugbreak();
    return 0;
}

#define FOLDHAUS_META_CPP
#endif // FOLDHAUS_META_CPP
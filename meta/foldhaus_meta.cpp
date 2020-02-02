//
// File: foldhaus_meta.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-19
//
#ifndef FOLDHAUS_META_CPP

#include "gs_meta.cpp"
#include "gs_meta_typeinfo_generator.h"

internal void
GenerateNodeMetaInfo (gsm_code_generator* NodeTypeGen, string_builder* CallNodeProcGen, gs_meta_preprocessor Meta)
{
    // TODO(Peter): Create a FilterTypesByTag function to create a contiguous array
    // of type_definition** 
    
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
                
                type_table_handle ReturnTypeHandle = Decl->Function.ReturnTypeHandle;
                type_definition* ReturnType = GetTypeDefinition(ReturnTypeHandle, Meta.TypeTable);
                
                WriteF(CallNodeProcGen, "        case NodeType_%.*s:\n", StringExpand(Decl->Identifier));
                WriteF(CallNodeProcGen, "        {\n");
                WriteF(CallNodeProcGen, "            %.*s(", StringExpand(Decl->Identifier));
                
                for (u32 j = 0; j < Decl->Function.Parameters.Used; j++)
                {
                    variable_decl* Param = Decl->Function.Parameters.GetElementAtIndex(j);
                    type_table_handle ParamTypeHandle = Param->TypeHandle;
                    type_definition* ParamType = GetTypeDefinition(ParamTypeHandle, Meta.TypeTable);
                    WriteF(CallNodeProcGen, "(%.*s*)NodeData", StringExpand(ParamType->Identifier));
                    if (j + 1 < Decl->Function.Parameters.Used)
                    {
                        WriteF(CallNodeProcGen, ", ");
                    }
                }
                WriteF(CallNodeProcGen, ");\n");
                WriteF(CallNodeProcGen, "        } break;\n");
            }
        }
    }
    WriteF(CallNodeProcGen, "    }\n");
    WriteF(CallNodeProcGen, "}\n\n");
    
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
    string_builder CallNodeProcGen = {0};
    GenerateNodeMetaInfo(&NodeTypeGen, &CallNodeProcGen, Meta);
    
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
        WriteStringBuilderToFile(CallNodeProcGen, NodeInfoH);
        fclose(NodeInfoH);
    }
    
    FinishMetaprogram(&Meta);
    
    //__debugbreak();
    return 0;
}

#define FOLDHAUS_META_CPP
#endif // FOLDHAUS_META_CPP
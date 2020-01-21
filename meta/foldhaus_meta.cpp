//
// File: foldhaus_meta.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-19
//
#ifndef FOLDHAUS_META_CPP

#include "gs_meta.cpp"
#include "gs_meta_typeinfo_generator.h"

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
    // TODO(Peter): Create a FilterTypesByTag function to create a contiguous array
    // of type_definition** 
    printf("\n\n");
    for (u32 b = 0; b < Meta.TypeTable.TypeBucketsCount; b++)
    {
        type_table_hash_bucket Bucket = Meta.TypeTable.Types[b];
        for (u32 i = 0; i < TYPE_TABLE_BUCKET_MAX; i++)
        {
            if (!Bucket.Keys[i] == 0) { continue; }
            
            type_definition* Decl = Bucket.Values + i;
            if (HasTag(MakeStringLiteral("node_proc"), Decl->MetaTags) &&
                Decl->Type == TypeDef_Function)
            {
                AddEnumElement(&NodeTypeGen, Decl->Identifier);
                
                type_table_handle ReturnTypeHandle = Decl->Function.ReturnTypeHandle;
                type_definition* ReturnType = GetTypeDefinition(ReturnTypeHandle, Meta.TypeTable);
                printf("%.*s %.*s(\n", StringExpand(ReturnType->Identifier), StringExpand(Decl->Identifier));
                for (u32 j = 0; j < Decl->Function.Parameters.Used; j++)
                {
                    variable_decl* Param = Decl->Function.Parameters.GetElementAtIndex(j);
                    type_table_handle ParamTypeHandle = Param->TypeHandle;
                    type_definition* ParamType = GetTypeDefinition(ParamTypeHandle, Meta.TypeTable);
                    printf("    %.*s %.*s,\n", StringExpand(ParamType->Identifier), StringExpand(Param->Identifier));
                }
                printf(");\n\n");
            }
        }
    }
    printf("\n\n");
    
    FinishEnumGeneration(&NodeTypeGen);
    
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
        fclose(NodeInfoH);
    }
    
    FinishMetaprogram(&Meta);
    //__debugbreak();
    return 0;
}

#define FOLDHAUS_META_CPP
#endif // FOLDHAUS_META_CPP
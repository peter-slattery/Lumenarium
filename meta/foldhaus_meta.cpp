//
// Usage
//
// GSMetaTag(<tag name>) to give commands to the meta layer
//
// Tag Values
// 
// breakpoint 
//   will cause the meta layer to break in the debugger when it reaches
//   that point in processing the file
//   TODO: specify which stage you want it to break at


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
    s64 Cycles_Preprocess = GetWallClock();
    
    typeinfo_generator TypeGenerator = InitTypeInfoGenerator(Meta.TypeTable);
    
    GenerateFilteredTypeInfo(MakeStringLiteral("node_struct"), Meta.TypeTable, &TypeGenerator);
    GenerateFilteredTypeInfo(MakeStringLiteral("gen_type_info"), Meta.TypeTable, &TypeGenerator);
    
    FinishGeneratingTypes(&TypeGenerator);
    
    FILE* TypeInfoH = fopen("C:\\projects\\foldhaus\\src\\generated\\gs_meta_generated_typeinfo.h", "w");
    if (TypeInfoH)
    {
        WriteStringBuilderToFile(TypeGenerator.TypeList, TypeInfoH);
        WriteStringBuilderToFile(TypeGenerator.StructMembers, TypeInfoH);
        WriteStringBuilderToFile(TypeGenerator.TypeDefinitions, TypeInfoH);
        fclose(TypeInfoH);
    }
    
    FinishMetaprogram(&Meta);
    //__debugbreak();
    return 0;
}
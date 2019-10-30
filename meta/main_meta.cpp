#include "..\src\gs_language.h"
#include "..\src\foldhaus_memory.h"
#include "..\src\gs_string.h"

#include "gs_meta.cpp"

#include <windows.h>
#include <stdio.h>

int main (int ArgCount, char* Arg[])
{
    gs_meta_processor Meta = CreateMetaProcessor();
    AddFile(&Meta, "C:/projects/foldhaus/meta/meta_test.cpp");
    
    LexAllFiles(&Meta);
    ParseAllFiles(&Meta);
    
    
#if 0
    identifier_table_entry* NodeStructEntry = AddDefineToIdentifierTable(&Meta, "NODE_STRUCT", "NODE_STRUCT(%name)", StructDefinition);
    identifier_table_entr* NodeProcEntry = AddDefineToIdentifierTable(&Meta, "NODE_PROC", "NODE_PROC(%name, %data)", FunctionDefinition);
    
    symbol_list NodeStructs = FindAllMatchingSymbols(Meta, NodeStructEntry);
    for (s32 i = 0; i < NodeStructs.Length; i++)
    {
        struct_definition_symbol Struct = NodeStructs.List[i];
        
        Struct.Size;
        
        for (s32 m = 0; m < Struct.MembersCount; m++)
        {
            Struct.Members[m];
        }
    }
#endif
    
    if (Meta.ErrorList.TotalUsed > 0)
    {
        PrintErrorList(Meta.ErrorList);
    }
    return 0;
}
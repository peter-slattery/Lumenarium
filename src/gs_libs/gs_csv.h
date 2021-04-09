/* date = March 24th 2021 5:53 pm */

#ifndef GS_CSV_H
#define GS_CSV_H

struct gscsv_cell
{
    gs_const_string Value;
};

struct gscsv_row
{
    u64* CellIndices;
};

struct gscsv_sheet
{
    char SeparatorChar;
    gscsv_cell* Cells;
    gscsv_row* Rows;
    u64 RowCount;
    u64 ColumnCount;
};

struct gscsv_sheet_desc
{
    char SeparatorChar;
};

struct gscsv_parser
{
    gs_const_string Str;
    u64 At;
    u64 Line;
};

internal void
CSVParser_Reset(gscsv_parser* Parser)
{
    Parser->At = 0;
}

internal char
CSVParser_CharAt(gscsv_parser Parser)
{
    char Result = Parser.Str.Str[Parser.At];
    return Result;
}

internal bool
CSVParser_CanAdvance(gscsv_parser Parser) {
    bool Result = Parser.At < Parser.Str.Length;
    return Result;
}

internal void
CSVParser_Advance(gscsv_parser* Parser)
{
    if (CSVParser_CanAdvance(*Parser)) {
        if (IsNewline(CSVParser_CharAt(*Parser)))
        {
            Parser->Line += 1;
        }
        Parser->At += 1;
    }
}

internal void
CSVParser_AdvancePastChar(gscsv_parser* Parser, char Char)
{
    while (CSVParser_CanAdvance(*Parser) && CSVParser_CharAt(*Parser) != Char)
    {
        CSVParser_Advance(Parser);
    }
    
    if (CSVParser_CanAdvance(*Parser))
    {
        Assert(CSVParser_CharAt(*Parser) == Char);
        CSVParser_Advance(Parser);
    }
}

internal u64
CSVParser_AdvancePastSeparatorOrNewline(gscsv_parser* Parser, char SeparatorChar)
{
    u64 PointBeforeSeparator = 0;
    
    while (CSVParser_CanAdvance(*Parser) && 
           !(CSVParser_CharAt(*Parser) == SeparatorChar ||
             IsNewline(CSVParser_CharAt(*Parser))))
    {
        CSVParser_Advance(Parser);
    }
    
    PointBeforeSeparator = Parser->At;
    
    if (CSVParser_CanAdvance(*Parser))
    {
        while(IsNewline(CSVParser_CharAt(*Parser))) {
            CSVParser_Advance(Parser);
        }
        if (CSVParser_CharAt(*Parser) == SeparatorChar)
        {
            CSVParser_Advance(Parser);
        }
    }
    
    return PointBeforeSeparator;
}

internal gscsv_cell
CSVCell_Init(gs_const_string File, u64 CellStart, u64 CellEnd)
{
    gscsv_cell Result = {};
    Result.Value = Substring(File, CellStart, CellEnd);
    return Result;
}

internal void
CSVRow_PushCell(gscsv_row* Row, u64 CellIndex, u64 ColumnIndex)
{
    Row->CellIndices[ColumnIndex] = CellIndex;
}

internal gscsv_sheet
CSV_Parse(gs_const_string File, gscsv_sheet_desc Desc, gs_memory_arena* Arena)
{
    gscsv_sheet Result = {};
    
    gscsv_parser Parser = {};
    Parser.Str = File;
    Parser.At = 0;
    Parser.Line = 0;
    
    // Count Tabs in first line
    u64 Columns = 0;
    while (CSVParser_CanAdvance(Parser) && !IsNewline(CSVParser_CharAt(Parser)))
    {
        char At = CSVParser_CharAt(Parser);
        if (At == Desc.SeparatorChar) {
            Columns += 1;
        }
        CSVParser_Advance(&Parser);
    }
    // NOTE(PS): Add one on the end because the last column won't end in a SeparatorChar,
    // it ends in a newline
    Columns += 1;
    CSVParser_Reset(&Parser);
    
    // Count New Lines
    u64 Rows = 0;
    while (CSVParser_CanAdvance(Parser))
    {
        char At = CSVParser_CharAt(Parser);
        if (IsNewline(At))
        {
            Rows++;
            while (CSVParser_CanAdvance(Parser) && IsNewline(CSVParser_CharAt(Parser)))
            {
                CSVParser_Advance(&Parser);
            }
        } else {
            CSVParser_Advance(&Parser);
        }
    }
    // NOTE(PS): Adding a row becuase the last row will just end in the EOF
    Rows += 1;
    CSVParser_Reset(&Parser);
    
    // Allocate Result
    Result.SeparatorChar = Desc.SeparatorChar;
    Result.RowCount = Rows;
    Result.ColumnCount = Columns;
    Result.Cells = PushArray(Arena, gscsv_cell, Result.RowCount * Result.ColumnCount);
    Result.Rows = PushArray(Arena, gscsv_row, Result.RowCount);
    
    // for rows, parse row
    //     for cells parse cells
    for (u64 r = 0; r < Result.RowCount; r++)
    {
        u64 RowIndex = r;
        gscsv_row* Row = Result.Rows + RowIndex;
        Row->CellIndices = PushArray(Arena, u64, Result.ColumnCount);
        
        for (u64 c = 0; c < Result.ColumnCount; c++)
        {
            u64 CellIndex = (r * Result.ColumnCount) + c;
            u64 CellStart = Parser.At;
            u64 CellEnd = CSVParser_AdvancePastSeparatorOrNewline(&Parser, Desc.SeparatorChar);
            
            Result.Cells[CellIndex] = CSVCell_Init(Parser.Str, CellStart, CellEnd);
            CSVRow_PushCell(Row, CellIndex, c);
        }
    }
    
    return Result;
}

internal gs_const_string
CSVSheet_GetCell(gscsv_sheet Sheet, u64 Column, u64 Row)
{
    gs_const_string Result = {};
    
    if (Row < Sheet.RowCount && Column < Sheet.ColumnCount)
    {
        u64 CellIndex = (Row * Sheet.ColumnCount) + Column;
        Result = Sheet.Cells[CellIndex].Value;
    }
    
    return Result;
}


#endif //GS_CSV_H

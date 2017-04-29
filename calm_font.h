#if !defined(CALM_FONT_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

struct font_glyph
{
    char UnicodeCodePoint;
    bitmap Bitmap;
};

enum font_quality
{
    FontQuality_Debug,
    FontQuality_Size,
    FontQuality_Count
};

//NOTE(oliver): For Spacing Table: First array is the _preceding_ glyph, the second array is the _postceding_ glyph

#define KERNING_TABLE_SIZE 128

#define CALM_FONT_ID 'FONT'
#pragma pack(push, 1)
struct font_file_header
{
    u32 MagicID;
};

//NOTE(oliver): these offsets are relative to the font header; it is like the RIFF file format
struct font_header
{
    char Name[255];
    u32 GlyphsAt;
    u32 SpacingAt;
    u32 LineAdvance;
    u32 RowCount; //NOTE() Symmetrical Grid
    u32 QualitiesOffset;
    
    u32 OffsetToNextFont;
    
};
#pragma pack(pop)

struct font
{
    char *Name;
    
    s32 *SpacingTable;
    u32 SpacingNum;
    
    u32 GlyphCount;
    font_glyph *Glyphs[128];
    u32 UnicodeToIndex[1028];
    u32 LineAdvance;
    r32 *Qualities;
    
    font *Next;
};

inline u32 GetLineAdvance(font *Font)
{
    u32 Result = Font->LineAdvance;
    return Result;
}

internal font *
LoadFontFile(char *FileName, game_memory *Memory, memory_arena *Arena)
{
    font *ResultFonts = {};
    
    size_t FontFileSize = Memory->PlatformFileSize(FileName);
    void *AllocatedMemory = PushSize(Arena, FontFileSize); 
    
    game_file_handle FontHandle = Memory->PlatformBeginFile("fonts.clm");
    
    file_read_result ReadResult = Memory->PlatformReadFile(FontHandle, AllocatedMemory, FontFileSize, 0);
    
    Memory->PlatformEndFile(FontHandle);
    
    if(ReadResult.Data)
    {
        u8 *FileContents = (u8 *)ReadResult.Data;
        
        font_file_header *FileHeader = (font_file_header *)FileContents;
        
        Assert(FileHeader->MagicID == CALM_FONT_ID);
        
        for(u32 FileIndex = sizeof(font_file_header);
            FileIndex < ReadResult.Size;
            )
        {
            font *ResultFont = PushStruct(Arena, font);
            ResultFont->Next = ResultFonts;
            ResultFonts = ResultFont;
            
            font_header *FontHeader = (font_header *)(FileContents + FileIndex);
            u8 * FontHeader_u8 = (u8 *)FontHeader;
            
            ResultFont->Name = FontHeader->Name;
            ResultFont->SpacingNum = FontHeader->RowCount;
            ResultFont->SpacingTable = (s32 *)(FontHeader_u8 + FontHeader->SpacingAt);
            ResultFont->LineAdvance = FontHeader->LineAdvance;
            ResultFont->Qualities = (r32 *)(FontHeader_u8 + FontHeader->QualitiesOffset);
            
            ResultFont->GlyphCount++; //NOTE(oliver): For null glyph 
            
            for(u32 Size = FontHeader->GlyphsAt;
                Size < FontHeader->SpacingAt;
                )
            {
                font_glyph *Glyph = (font_glyph *)(FontHeader_u8 + Size);
                
                Size += sizeof(font_glyph);
                
                Glyph->Bitmap.Bits = (void *)(FontHeader_u8 + Size);
                
                Assert(ResultFont->GlyphCount < ArrayCount(ResultFont->Glyphs));
                u32 Index = ResultFont->GlyphCount++;
                ResultFont->Glyphs[Index] = Glyph;
                
                Assert(Glyph->UnicodeCodePoint < ArrayCount(ResultFont->UnicodeToIndex));
                ResultFont->UnicodeToIndex[Glyph->UnicodeCodePoint] = Index;
                
                
                Size += (Glyph->Bitmap.Pitch * Glyph->Bitmap.Height);
            }
            
            FileIndex += FontHeader->OffsetToNextFont;
            
        }
    }
    
    return ResultFonts;
    
}

internal bitmap *
GetFontGlyphBitmap(font *Font, char UnicodePoint)
{
    bitmap *Result = 0;
    
    Assert(UnicodePoint < ArrayCount(Font->UnicodeToIndex));
    u32 Index = Font->UnicodeToIndex[UnicodePoint];
    
    if(Index != 0)
    {
        font_glyph *Glyph = Font->Glyphs[Index];
        Assert(Glyph->UnicodeCodePoint == UnicodePoint);
        
        Result = &Glyph->Bitmap;
    }
    
    return Result;
}

internal s32 GetGlyphWidth(font *Font, char UnicodePoint) {
    bitmap *GlyphBitmap = GetFontGlyphBitmap(Font, UnicodePoint);
    s32 Result = 0;
    if(GlyphBitmap) {
        Result = GlyphBitmap->Width;
    } 
    
    return Result;
}

internal s32 
GetCharacterAdvanceFor(font *Font, char LastCodePoint, char ThisCodePoint)
{
    s32 Result = 0;
    if(LastCodePoint != 0 && (u32)LastCodePoint < Font->SpacingNum && (u32)ThisCodePoint < Font->SpacingNum)
    {
        Result = *(Font->SpacingTable + (LastCodePoint*Font->SpacingNum) + ThisCodePoint);
    }
    return Result;
}

inline void AdvanceCursor(font *Font, char LastCodePoint, char CheesePoint, s32 *XCursor, s32 *YCursor, u32 BufferWidth, r32 LineAdvanceModifier, b32 NewLineSensitive) {
    
    s32 XAdvanceForLetter = GetCharacterAdvanceFor(Font, LastCodePoint, CheesePoint);
    if((XAdvanceForLetter + *XCursor) > (s32)BufferWidth || CheesePoint == '\n' && NewLineSensitive)
    {
        *XCursor = 0;
        *YCursor += LineAdvanceModifier*GetLineAdvance(Font);
    }
    else
    {
        *XCursor += XAdvanceForLetter;
    }
    
}

internal void DrawBitmap(bitmap *Buffer, bitmap *Bitmap, s32 XOrigin_, s32 YOrigin_, rect2 Bounds, v4 Color);


// TODO(OLIVER): Compress this into a more reusable component i.e. text metrics and draw text.
internal v2i
TextToOutput(bitmap *Buffer, font *Font, char *String, s32 XCursor, s32 YCursor, rect2 Bounds, v4 Color, b32 DrawText = true, r32 LineAdvanceModifier = -1,  u32 OptionalLetterCount = INT_MAX, b32 NewLineSensitive = true) {
    //NOTE(OLIVER): We had this so we can move upwards when writing text
    char LastCodePoint = 0;
    
    u32 Count = 0;
    for(char *Letter = String;
        *Letter && Count < OptionalLetterCount;
        Letter++, Count++)
    {
        char CheesePoint = *Letter; // TODO(OLIVER): Handle UTF-8 strings!
        
        AdvanceCursor(Font, LastCodePoint, CheesePoint, &XCursor, &YCursor, Buffer->Width, LineAdvanceModifier, NewLineSensitive);
        if(DrawText) {
            DrawBitmap(Buffer, GetFontGlyphBitmap(Font, CheesePoint), XCursor, YCursor, Bounds, Color);
        }
        LastCodePoint = CheesePoint;                
    }
    
    AdvanceCursor(Font, LastCodePoint, '\0', &XCursor, &YCursor, Buffer->Width, LineAdvanceModifier, NewLineSensitive);
    
    v2i CursorP = V2int(XCursor, YCursor);
    return  CursorP;
}

internal rect2
GetTextBounds(font *Font, char *String)
{
    rect2 Bounds = InvertedInfinityRectangle();
    
    u32 LastCodePoint = 0;
    u32 XCursor = 0;
    u32 YCursor = 0;
    
    for(char *Letter = String;
        *Letter;
        ++Letter)
    {
        XCursor += GetCharacterAdvanceFor(Font, (char)LastCodePoint, *Letter);
        bitmap *CharacterBitmap = GetFontGlyphBitmap(Font, *Letter);
        rect2 CharacterBounds;
        CharacterBounds.Min = V2i(XCursor, YCursor);
        CharacterBounds.Max = V2i(CharacterBitmap->Width, CharacterBitmap->Height) + CharacterBounds.Min;
        
        Bounds = Union(Bounds, CharacterBounds);
        
        LastCodePoint = *Letter;
        
    }
    
    return Bounds;
}

//NOTE(): The qualities at the moment are between 0-1.0. Maybe the could be more 
//descriptive in the future. 9/2/2017
internal font *
FindFont(font *Fonts, r32 *Qualities)
{
    font *Result = 0;
    r32 BestMatch = 1000000.0f;
    
    for(font *Font = Fonts;
        Font;
        Font = Font->Next)
    {
        r32 SqrDifferenceTotal = 0;
        for(u32 QualityIndex = 0;
            QualityIndex < FontQuality_Count;
            ++QualityIndex)
        {
            SqrDifferenceTotal += Sqr(Abs(Qualities[QualityIndex] - Font->Qualities[QualityIndex]));
        }
        
        if(BestMatch > SqrDifferenceTotal)
        {
            Result = Font;
            BestMatch = SqrDifferenceTotal;
        }
    }
    return Result;
}

#define CALM_FONT_H
#endif

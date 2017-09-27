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
    
    //
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

enum quality_type {
    Font_Quality_Real32,
    Font_Quality_String, 
};

struct font_quality_value  {
    quality_type Type;
    union {
        r32 Value;
        char Name[256];
    };
};

struct font
{
    char *Name;
    
    s32 *SpacingTable;
    u32 SpacingNum;
    
    u32 GlyphCount;
    font_glyph *Glyphs[128];
    u32 UnicodeToIndex[1028];
    u32 LineAdvance;
    font_quality_value *Qualities;
    
    font *Next;
};

inline font_quality_value InitFontQualityValue(r32 Value) { 
    return {Font_Quality_Real32, Value}; 
}

inline font_quality_value InitFontQualityValue(char *Value) { 
    font_quality_value Result = {};
    Result.Type = Font_Quality_String;
    CopyStringToBuffer(Result.Name, ArrayCount(Result.Name), Value);
    return Result; 
}

inline u32 GetLineAdvance_(font *Font)
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
            ResultFont->Qualities = (font_quality_value *)(FontHeader_u8 + FontHeader->QualitiesOffset);
            
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

internal r32 GetLineAdvance(font *Font, r32 Scale, r32 LineAdvanceModifier) {
    r32 Result = Scale*LineAdvanceModifier*GetLineAdvance_(Font);
    return Result;
}

inline void AdvanceCursor(font *Font, char LastCodePoint, char CheesePoint, r32 *XCursor, r32 *YCursor, rect2 Bounds, r32 LineAdvanceModifier, b32 NewLineSensitive, transform *Transform) {
    // NOTE(OLIVER): This is to take into account different transforms i.e. rendering ortho and perspective. 
    // TODO(OLIVER): Do we want to handle transforms in a more uniform way 
    v2 Scale = V2(1.0f / Transform->Scale.X, 1.0f / Transform->Scale.Y);
    
    r32 XAdvanceForLetter = Scale.X*GetCharacterAdvanceFor(Font, LastCodePoint, CheesePoint);
    if((XAdvanceForLetter + *XCursor) >= Bounds.Max.X || (CheesePoint == '\n' && NewLineSensitive))
    {
        *XCursor = Bounds.Min.X;
        *YCursor += GetLineAdvance(Font, Scale.Y, LineAdvanceModifier);
    }
    else
    {
        *XCursor += XAdvanceForLetter;
    }
    
}

//This check is with y-axis pointing up: so for Y it is actually a min check. Maybe I should try the ortho projection?
inline void AdvanceCursor(font *Font, char LastCodePoint, char CheesePoint, r32 *XCursor, r32 *YCursor, rect2 Bounds, r32 LineAdvanceModifier, b32 NewLineSensitive, transform *Transform, r32 *XCursorMax, r32 *YCursorMax) {
    
    AdvanceCursor(Font, LastCodePoint, CheesePoint,XCursor, YCursor, Bounds, LineAdvanceModifier, NewLineSensitive, Transform);
    
    if(*XCursor > *XCursorMax) {
        *XCursorMax = *XCursor;
    }
    if(*YCursor < *YCursorMax) {
        *YCursorMax = *YCursor;
    }
    
}

internal void DrawBitmap(bitmap *Buffer, bitmap *Bitmap, s32 XOrigin_, s32 YOrigin_, rect2 Bounds, v4 Color);

struct draw_text_options {
    b32 DisplayText; 
    r32 LineAdvanceModifier;
    u32 OptionalLetterCount;
    b32 NewLineSensitive;
    r32 ZDepth;
    b32 AdvanceYAtStart;
    b32 MaxCursorPos; //Returns the max position the cursor reached. 
};

inline draw_text_options InitDrawOptions() {
    draw_text_options Result = {};
    Result.DisplayText = true; 
    Result.LineAdvanceModifier = -1;
    Result.OptionalLetterCount = INT_MAX;
    Result.NewLineSensitive = true;
    Result.ZDepth = 0.0f;
    Result.AdvanceYAtStart = false;
    Result.MaxCursorPos = false;
    
    return Result;
}

// TODO(OLIVER): Compress this into a more reusable component i.e. text metrics and draw text.
internal v2i
TextToOutput(render_group *Group, font *Font, char *String, r32 XCursor, r32 YCursor, rect2 Bounds, v4 Color, draw_text_options *Options) {
    char LastCodePoint = 0;
    r32 MaxXCursor = XCursor;
    r32 MaxYCursor = YCursor;
    
    if(Options->AdvanceYAtStart) {
        // NOTE(OLIVER): Remember the line advance is normally -1 for text being written downwards. So we add the line advance to move downwards. 
        YCursor += GetLineAdvance(Font, 1.0f/Group->Transform.Scale.Y, Options->LineAdvanceModifier);
    }
    
    u32 Count = 0;
    for(char *Letter = String;
        *Letter && Count < Options->OptionalLetterCount;
        Letter++, Count++)
    {
        char CheesePoint = *Letter; // TODO(OLIVER): Handle UTF-8 strings!
        
        AdvanceCursor(Font, LastCodePoint, CheesePoint, &XCursor, &YCursor, Bounds, Options->LineAdvanceModifier, Options->NewLineSensitive, &Group->Transform, &MaxXCursor, &MaxYCursor);
        if(Options->DisplayText) {
            bitmap *FontGlyphBitmap = GetFontGlyphBitmap(Font, CheesePoint);
            if(FontGlyphBitmap) {
                PushBitmapScale(Group, V3(XCursor, YCursor, Options->ZDepth),FontGlyphBitmap, 1.0f, Transform(&Group->Transform, Bounds), Color);
            }
        }
        LastCodePoint = CheesePoint;                
    }
    
    AdvanceCursor(Font, LastCodePoint, '\0', &XCursor, &YCursor, Bounds, Options->LineAdvanceModifier, Options->NewLineSensitive, &Group->Transform, &MaxXCursor, &MaxYCursor);
    
    v2i CursorP = V2int((s32)XCursor, (s32)YCursor);
    
    if(Options->MaxCursorPos) {
        CursorP = V2int((s32)MaxXCursor, (s32)MaxYCursor);
    } 
    
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
//Added quality types but not currently using them.

struct find_font_result {
    font *Font;
    r32 BestMatch;
};

inline void InnerFindFont(font *Font, font_quality_value *Qualities, find_font_result *Res) {
    r32 SqrDifferenceTotal = 0;
    for(u32 QualityIndex = 0;
        QualityIndex < FontQuality_Count;
        ++QualityIndex)
    {
        switch(Qualities[QualityIndex].Type) {
            case Font_Quality_Real32: {
                SqrDifferenceTotal += Sqr(Abs(Qualities[QualityIndex].Value - Font->Qualities[QualityIndex].Value));
            } break;
            case Font_Quality_String: {
                //Do we need these extra cases now?
            } break;
        }
    }
    
    if(Res->BestMatch > SqrDifferenceTotal)
    {
        Res->Font = Font;
        Res->BestMatch = SqrDifferenceTotal;
    }
} 

internal font *
FindFont(font *Fonts, font_quality_value *Qualities) {
    find_font_result Result = {};
    Result.BestMatch = 1000000.0f;
    
    for(font *Font = Fonts; Font; Font = Font->Next) {
        InnerFindFont(Font, Qualities, &Result);
    }
    return Result.Font;
}

internal font *
FindFont_(font **Fonts, font_quality_value *Qualities, u32 FontCount) {
    find_font_result Result = {};
    Result.BestMatch = 1000000.0f;
    
    forN_(FontCount, i) {
        InnerFindFont(Fonts[i], Qualities, &Result);
    }
    return Result.Font;
}

internal font *
FindFontByName(font *Fonts, char *FontName, font_quality_value *Qualities = 0)
{
    font *Result = 0;
    
    u32 FontCount = 0;
    font *FontPtrs[64] = {};
    
    font *LastFont = 0;
    for(font *Font = Fonts;
        Font;
        Font = Font->Next)
    {
        if(DoStringsMatch(FontName, Font->Name)){
            Assert(FontCount < ArrayCount(FontPtrs));
            FontPtrs[FontCount++] = Font;
        }
    }
    
    if(Qualities) {
        Result = FindFont_(FontPtrs, Qualities, FontCount);
    } else if(FontCount){
        Result = FontPtrs[0];
    }
    
    Assert(Result);
    
    return Result;
}

#define CALM_FONT_H
#endif

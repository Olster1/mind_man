#if !defined(WRITE_FONT_FILE_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */


#define MAX_BITMAP_WIDTH 32
#define MAX_BITMAP_HEIGHT MAX_BITMAP_WIDTH
#include "calm_font.h"

internal void
Win32WriteFontFile()
{
    HDC DeviceContext = CreateCompatibleDC(0);
    
    BITMAPINFO BitmapInfo = {};
    BitmapInfo.bmiHeader = {};
    BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    BitmapInfo.bmiHeader.biWidth = MAX_BITMAP_WIDTH;
    BitmapInfo.bmiHeader.biHeight = MAX_BITMAP_HEIGHT;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;
    u32 DC_BitmapPitch = MAX_BITMAP_WIDTH*sizeof(u32);
    
    u32 DC_Size = DC_BitmapPitch * MAX_BITMAP_HEIGHT;
    
    void *DCBits = VirtualAlloc(0, DC_Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    HBITMAP DIBSection = CreateDIBSection(DeviceContext,
                                          &BitmapInfo,
                                          DIB_RGB_COLORS,
                                          &DCBits,
                                          0,
                                          0);
    
    char *FileNameFontFile = "fonts.clm";
    HANDLE FileHandle;
    for(;;)
    {
        FileHandle = CreateFile(
            FileNameFontFile,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            0,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            0);
        
        if(GetLastError() == ERROR_ALREADY_EXISTS)
        {
            CloseHandle(FileHandle);
            DeleteFile(FileNameFontFile);
        }
        else
        {
            break;
        }
        
    }
    
    font_file_header File_Header = {};
    File_Header.MagicID = CALM_FONT_ID;
    
    DWORD NumberOfBytesWritten;
    
    WriteFile(FileHandle, &File_Header, sizeof(font_file_header), &NumberOfBytesWritten, 0);
    Assert(NumberOfBytesWritten == sizeof(font_file_header));
    
    
    char *FontNames[] = {"Liberation Mono", "Arial", "Inconsolata", "Karmina Regular"}; 
    char *FileNames[] = {"c:/Windows/Fonts/LiberationMono-Regular.ttf",
        "c:/Windows/Fonts/arial.ttf", "c:/Windows/Fonts/Inconsolata.ttf","c:/Windows/Fonts/Karmina-Regular.ttf"};
    
    font_quality_value Qualities[ArrayCount(FileNames)][FontQuality_Count];
    Qualities[0][FontQuality_Debug] = InitFontQualityValue(1.0f);
    Qualities[1][FontQuality_Debug] = InitFontQualityValue(0.0f);
    Qualities[2][FontQuality_Debug] = InitFontQualityValue(1.0f);
    Qualities[3][FontQuality_Debug] = InitFontQualityValue(0.0f);
    
    Qualities[0][FontQuality_Size] = InitFontQualityValue(1.0f);
    Qualities[1][FontQuality_Size] = InitFontQualityValue(1.0f);
    Qualities[2][FontQuality_Size] = InitFontQualityValue(1.0f);
    Qualities[3][FontQuality_Size] = InitFontQualityValue(1.0f);
    
    u32 FontSize = MAX_BITMAP_HEIGHT;
    
    u32 ThisFontOffset =  sizeof(font_file_header);
    
    for(u32 FontFileIndex = 0;
        FontFileIndex < ArrayCount(FontNames);
        ++FontFileIndex)
    {
        SetFilePointer(FileHandle, sizeof(font_header) + ThisFontOffset, 0, FILE_BEGIN);
        
        char *FontName = FontNames[FontFileIndex];
        char *FileName = FileNames[FontFileIndex];
        
        AddFontResourceEx(FileName, FR_PRIVATE, 0);
        
        HFONT Win32Font = CreateFont(FontSize, 0, 0, 0,
                                     FW_NORMAL, //NOTE(oliver): Weight
                                     false, //NOTE(oliver): Italics
                                     false, //NOTE(oliver): Underline
                                     false, //NOTE(oliver): Strikeout
                                     DEFAULT_CHARSET,
                                     OUT_DEFAULT_PRECIS,
                                     CLIP_DEFAULT_PRECIS,
                                     ANTIALIASED_QUALITY,
                                     DEFAULT_PITCH | FF_DONTCARE,
                                     FontName);
        
        SelectObject(DeviceContext, DIBSection);
        SelectObject(DeviceContext, Win32Font);
        SetBkColor(DeviceContext, RGB(255.0f, 255.0f, 255.0f));
        
        TEXTMETRIC TextMetrics;
        GetTextMetrics(DeviceContext, &TextMetrics);
        
        s32 Spacing[KERNING_TABLE_SIZE][KERNING_TABLE_SIZE] = {};
        
        u32 FontGlyphDataSize = 0;
        
        u32 KerningPairCount = GetKerningPairs(DeviceContext, 0, 0);
        KERNINGPAIR *KerningPairs = (KERNINGPAIR *)VirtualAlloc(0, sizeof(KERNINGPAIR)*KerningPairCount, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        
        GetKerningPairs(DeviceContext, KerningPairCount, KerningPairs);
        
        r32 Inv_MaxBitmapHeight = 1.0f / MAX_BITMAP_HEIGHT;
        
        for(u32 UnicodePoint = ' ';
            UnicodePoint <= '~';
            ++UnicodePoint)
        {
            char *CheesePoint = (char *)(&UnicodePoint);
            
            if(*CheesePoint == ' ')
            {
                BreakPoint;
            }
            
            memset(DCBits, 0, DC_Size);
            u32 Size_ = DC_Size;
            while(Size_--)
            {
                *((u8 *)DCBits) = 0x33;
            }
            
            TextOut(DeviceContext,
                    0,
                    0,
                    CheesePoint,
                    1);
            
            
            ABC ABC_Widths;
            GetCharABCWidths(DeviceContext, UnicodePoint, UnicodePoint, &ABC_Widths);
            
            for(u32 Index = 0;
                Index < ArrayCount(Spacing);
                ++Index)
            {
                Spacing[UnicodePoint][Index] += ABC_Widths.abcC + ABC_Widths.abcB;
                Spacing[Index][UnicodePoint] += ABC_Widths.abcA;
                
                for(u32 KerningIndex = 0;
                    KerningIndex < KerningPairCount;
                    ++KerningIndex)
                {
                    KERNINGPAIR *Pair = KerningPairs + KerningIndex;
                    if(Pair->wFirst == UnicodePoint && Pair->wSecond == Index)
                    {
                        Spacing[UnicodePoint][Index] += Pair->iKernAmount;
                        break;
                    }
                }
                
                
            }
            
            //NOTE(oliver): Find boundary conditions
            u32 MinX = MAX_BITMAP_WIDTH;
            u32 MaxX = 0;
            u32 MinY = MAX_BITMAP_HEIGHT;
            u32 MaxY = 0;
            //TODO(oliver): Is there a better way to do this? Like setting the undrawn region to not black
            u32 ABC_Width = ABC_Widths.abcA + ABC_Widths.abcB + ABC_Widths.abcC;
            
            u32 YOffset = 1; //There is an implicit line of white pixels draw 
            u32 XOffset = 0;
            u8 *Source_u8 = (u8 *)DCBits + (YOffset*DC_BitmapPitch) + XOffset*sizeof(u32);
            
            for(u32 Y = YOffset;
                Y < MAX_BITMAP_HEIGHT;
                ++Y)
            {
                u32 *Source = (u32 *)Source_u8;
                
                for(u32 X = XOffset;
                    X < ABC_Width;
                    ++X)
                {
                    u32 Color = *Source++;
                    if(Color == 0x000000)
                    
                    {
                        if(X > MaxX) { MaxX = X;} 
                        if(X < MinX) { MinX = X;}
                        if(Y > MaxY) { MaxY = Y;} 
                        if(Y < MinY) { MinY = Y;}
                    }                                
                    
                }
                
                Source_u8 += DC_BitmapPitch;
                
            }
            
            
            font_glyph Glyph;
            Glyph.UnicodeCodePoint = *CheesePoint;
            Glyph.Bitmap.Width =  Max(0, (s32)(MaxX + 1) - (s32)(MinX));
            Glyph.Bitmap.Height = Max(0, (s32)(MaxY + 1) - (s32)(MinY));
            Glyph.Bitmap.Pitch = sizeof(u32)*Glyph.Bitmap.Width;
            if(Glyph.Bitmap.Height)
            {
                Glyph.Bitmap.AlignPercent = {0, ((r32)(-(r32)MinY + TextMetrics.tmDescent) / (r32)Glyph.Bitmap.Height)};
                
            }
            else
            {
                Glyph.Bitmap.AlignPercent = {};
            }
            Assert(Glyph.Bitmap.Width <= 128);
            
            u32 BitsSize = Glyph.Bitmap.Pitch * Glyph.Bitmap.Height;
            Glyph.Bitmap.Bits = VirtualAlloc(0, BitsSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            
            FontGlyphDataSize += BitsSize + sizeof(font_glyph);
            
            //NOTE(oliver): Copy bits to make smaller bitmap
            
            {
                u8 *Dest_u8 = (u8 *)Glyph.Bitmap.Bits;
                Source_u8 = (u8 *)DCBits;
                
                for(u32 Y = 0;
                    Y < Glyph.Bitmap.Height;
                    ++Y)
                {
                    u32 *Source = (u32 *)Source_u8;
                    u32 *Dest = (u32 *)Dest_u8;
                    
                    Source += (MinY*MAX_BITMAP_WIDTH) + MinX;
                    
                    for(u32 X = 0;
                        X < Glyph.Bitmap.Width;
                        ++X)
                    {
                        u32 SC = *Source;
                        
                        if(SC == 0x000000)
                        {
                            SC = 0xFFFFFFFF;
                        }
                        else
                        {
                            SC = 0;
                        }
                        
                        *Dest = SC;
                        
                        Dest++;
                        Source++;
                    }
                    Dest_u8 += Glyph.Bitmap.Pitch; 
                    Source_u8 += DC_BitmapPitch;
                    
                }
            }
            
            
            WriteFile(FileHandle,
                      &Glyph,
                      sizeof(font_glyph),
                      &NumberOfBytesWritten,
                      0);
            
            Assert(NumberOfBytesWritten == sizeof(font_glyph));
            
            WriteFile(FileHandle,
                      Glyph.Bitmap.Bits,
                      BitsSize,
                      &NumberOfBytesWritten,
                      0);
            
            Assert(NumberOfBytesWritten == BitsSize);
            
        }
        
        OUTLINETEXTMETRIC OutlineTextMetric;
        GetOutlineTextMetrics(DeviceContext, sizeof(OUTLINETEXTMETRIC), &OutlineTextMetric);
        
        u32 SpacingTableSize = sizeof(u32)*ArrayCount(Spacing)*ArrayCount(Spacing);
        
        font_header Header = {};
        
        CopyStringToBuffer(Header.Name, ArrayCount(Header.Name), FontName);
        Header.GlyphsAt = sizeof(font_header);
        Header.SpacingAt = FontGlyphDataSize + Header.GlyphsAt;
        Header.LineAdvance = OutlineTextMetric.otmLineGap + TextMetrics.tmHeight;
        Header.RowCount = KERNING_TABLE_SIZE;
        Header.QualitiesOffset = Header.SpacingAt + SpacingTableSize;
        
        Header.OffsetToNextFont = Header.QualitiesOffset + sizeof(Qualities[FontFileIndex]);
        
        SetFilePointer(FileHandle, ThisFontOffset, 0, FILE_BEGIN);
        
        WriteFile(FileHandle,
                  &Header,
                  sizeof(font_header),
                  &NumberOfBytesWritten,
                  0);
        
        Assert(NumberOfBytesWritten == sizeof(font_header));
        
        SetFilePointer(FileHandle, ThisFontOffset + Header.SpacingAt, 0, FILE_BEGIN);
        
        WriteFile(FileHandle,
                  Spacing,
                  SpacingTableSize,
                  &NumberOfBytesWritten,
                  0);
        
        Assert(NumberOfBytesWritten == sizeof(u32)*KERNING_TABLE_SIZE*KERNING_TABLE_SIZE);
        
        SetFilePointer(FileHandle, ThisFontOffset + Header.QualitiesOffset, 0, FILE_BEGIN);
        
        WriteFile(FileHandle,
                  &Qualities[FontFileIndex],
                  sizeof(Qualities[FontFileIndex]),
                  &NumberOfBytesWritten,
                  0);
        
        Assert(NumberOfBytesWritten == sizeof(Qualities[FontFileIndex]));
        
        ThisFontOffset += Header.OffsetToNextFont;
        
        VirtualFree(KerningPairs, 0, MEM_RELEASE);
        
    }
    
    VirtualFree(DCBits, 0, MEM_RELEASE);
    CloseHandle(FileHandle);
}

//NOTE(oliver): These structs are defined in the platform layer
/*
  struct font_glyph
  {
  char UnicodeCodePoint;
  bitmap Bitmap;
  };
  
//NOTE(oliver): First array is the _preceding_ glyph, the second array is the _postceding_ glyph

#define KERNING_TABLE_SIZE 128

#define CALM_FONT_ID 'FONT'
#pragma pack(push, 1)
struct font_file_header
{
u32 MagicID;
u32 GlyphAt;
u32 SpacingAt;
u32 RowCount; //NOTE() Symmetrical Grid
};
#pragma pack(pop)
*/

#define WRITE_FONT_FILE_H
#endif

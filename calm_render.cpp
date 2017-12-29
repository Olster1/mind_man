internal void
ClearBitmap(bitmap *Bitmap, v4 Color01) {
    
    u32 Color255 = ((u32)(255.0f*Color01.A) << 24 |
                    (u32)(255.0f*Color01.R) << 16 |
                    (u32)(255.0f*Color01.G) << 8 |
                    (u32)(255.0f*Color01.B) << 0);
    
    
    u32 *Bits = (u32 *)Bitmap->Bits;
    
    for(u32 Y = 0;
        Y < Bitmap->Height;
        ++Y)
    {
        for(u32 X = 0;
            X < Bitmap->Width;
            ++X)
        {
            *Bits++ = Color255;
        }
    }
    
}

internal void
BltRectangle(bitmap *Bitmap, s32 XCorner, s32 YCorner, s32 Width, s32 Height, v4 Color01)
{
    
    u32 Color255 = ((u32)(255.0f*Color01.A) << 24 |
                    (u32)(255.0f*Color01.R) << 16 |
                    (u32)(255.0f*Color01.G) << 8 |
                    (u32)(255.0f*Color01.B) << 0);
    
    s32 X1 = XCorner;
    s32 Y1 = YCorner;
    s32 X2 = XCorner + Width;
    s32 Y2 = YCorner + Height;
    
    s32 XMin = Min(X1, X2);
    s32 XMax = Max(X1, X2);
    s32 YMin = Min(Y1, Y2);
    s32 YMax = Max(Y1, Y2);
    
    if(XMin < 0) { XMin = 0; }
    if(YMin < 0) { YMin = 0; }
    
    if(XMax > (s32)Bitmap->Width) { XMax = Bitmap->Width; }
    if(YMax > (s32)Bitmap->Height) { YMax = Bitmap->Height; }
    
    u8 *Bits = (u8 *)Bitmap->Bits + YMin*Bitmap->Pitch + XMin*sizeof(u32);
    
    for(s32 Y = YMin;
        Y < YMax;
        ++Y)
    {
        u32 *Pixel = (u32 *)Bits;
        for(s32 X = XMin;
            X < XMax;
            ++X)
        {
            *Pixel++ = Color255;
        }
        Bits += Bitmap->Pitch;
    }
    
}

inline u32
Color01AsV4To255AsU32(v4 Color01)
{
    u32 Color255 = ((u32)(255.0f*Color01.A) << 24 |
                    (u32)(255.0f*Color01.R) << 16 |
                    (u32)(255.0f*Color01.G) << 8 |
                    (u32)(255.0f*Color01.B) << 0);
    
    return Color255;
    
}

internal void
FillRectangle(bitmap *Bitmap, s32 XCorner, s32 YCorner, s32 Width, s32 Height, u32 Color255)
{
    s32 X1 = XCorner;
    s32 Y1 = YCorner;
    s32 X2 = XCorner + Width;
    s32 Y2 = YCorner + Height;
    
    s32 XMin = Min(X1, X2);
    s32 XMax = Max(X1, X2);
    s32 YMin = Min(Y1, Y2);
    s32 YMax = Max(Y1, Y2);
    
    if(XMin < 0) { XMin = 0; }
    if(YMin < 0) { YMin = 0; }
    
    if(XMax > (s32)Bitmap->Width) { XMax = Bitmap->Width; }
    if(YMax > (s32)Bitmap->Height) { YMax = Bitmap->Height; }
    
    u8 *Bits = (u8 *)Bitmap->Bits + YMin*Bitmap->Pitch + XMin*sizeof(u32);
    
    for(s32 Y = YMin;
        Y < YMax;
        ++Y)
    {
        u32 *Pixel = (u32 *)Bits;
        for(s32 X = XMin;
            X < XMax;
            ++X)
        {
            *Pixel++ = Color255;
        }
        Bits += Bitmap->Pitch;
    }
    
}
internal void
FillRectangle(bitmap *Bitmap, s32 XCorner, s32 YCorner, s32 Width, s32 Height, v4 Color01)
{
    u32 Color255 = Color01AsV4To255AsU32(Color01);
    
    FillRectangle(Bitmap, XCorner, YCorner, Width, Height, Color255);
}

internal void
FillRectangle(bitmap *Bitmap, rect2 Rect, v4 Color01)
{
    FillRectangle(Bitmap, (s32)Rect.MinX, (s32)Rect.MinY, (s32)(Rect.MaxX - Rect.MinX), (s32)(Rect.MaxY - Rect.MinY), Color01);
}


#pragma pack(push, 1)
struct bitmap_header
{
    u16 MagicID;             
    u32 FileSize;            
    u16 Reserved1, Reserved2;
    u32 DataOffsetIntoFile;        
    u32 HeaderSize;        
    s32 Width, Height;     
    u16 ColorPlanes;       
    u16 BitsPerPixel;       
    u32 Compression;        
    u32 ImageSize;          
    
};
#pragma pack(pop)

inline u32 PackRGBA(v4 Color01) {
    
    Color01 = 255.0f * Color01;
    u32 Result = ((u32)Color01.A << 24 |
                  (u32)Color01.R << 16 |
                  (u32)Color01.G << 8 |
                  (u32)Color01.B << 0
                  );
    return Result;
    
}

inline void InitBitmap(bitmap *Bitmap, s32 Width, s32 Height, s32 SizeOfPixel, v2 AlignPercent) {
    Bitmap->Width = Width;
    Bitmap->Height = Height;
    Bitmap->Pitch = Width * SizeOfPixel;
    Bitmap->AlignPercent = AlignPercent;
    Bitmap->Handle = 0;
    Bitmap->LoadState = RESOURCE_NOT_LOADED;
}

internal bitmap
CreateNewBitmap(memory_arena *Arena, u32 Width, u32 Height, b32 Clear, v4 ClearColor)
{
    bitmap Result;
    
    InitBitmap(&Result, Width, Height, sizeof(u32), V2(0.5f, 0.5f));
    
    u32 MemorySize = Width*Height*sizeof(u32);
    Result.Bits = PushSize(Arena, MemorySize);
    
    if(Clear) {
        ClearMemoryU32(Result.Bits, Width*Height, PackRGBA(ClearColor));
    }
    
    return Result;
}

internal bitmap
LoadBitmap(game_memory *Memory, memory_arena *MemoryArena, char *FileName, v2 AlignPercent = V2(0.5f, 0.5f)) {
    bitmap Result = {};
    
    file_read_result FileContents = Memory->PlatformReadEntireFile(FileName);
    
    if(FileContents.Data)
    {
        bitmap_header *BitmapHeader = (bitmap_header *)FileContents.Data;
        Assert(BitmapHeader->MagicID == 'MB');
        Assert(BitmapHeader->BitsPerPixel == 8 || BitmapHeader->BitsPerPixel == 32);
        Assert(BitmapHeader->Compression == 0);
        
        InitBitmap(&Result, BitmapHeader->Width, BitmapHeader->Height, sizeof(u32), AlignPercent);
        
        Result.Bits = (void *)((u8 *)BitmapHeader + BitmapHeader->DataOffsetIntoFile);
        
        
        if(BitmapHeader->BitsPerPixel == 8)
        {
            //This supports 8 bits per pixel images allowing them to be mapped to a 32bit/pixel space. 
            if(MemoryArena) {
                u32 *NewBits = PushArray(MemoryArena, u32, Result.Width*Result.Height);
                u32 Size  = Result.Width*Result.Height*sizeof(u8);
                
                u8 *NewBits_u8 = (u8 *)NewBits;
                u8 *OldBits_u8 = (u8 *)Result.Bits;
                while(Size--)
                {
                    u8 Value = *OldBits_u8++;
                    *NewBits_u8++ = 255 - Value;
                    *NewBits_u8++ = 255 - Value;
                    *NewBits_u8++ = 255 - Value;
                    *NewBits_u8++ = 255;
                    
                }
                
                Result.Bits = NewBits;
            }
        } else { //pre-multiplied alpha
            u8 *At = (u8 *)Result.Bits;
            for(s32 i = 0; i < (s32)(Result.Width*Result.Height); ++i) {
                r32 Alpha = (r32)(At[3])/255.0f;
                for(int colorIndex = 0; colorIndex < 3; ++colorIndex) {
                    At[colorIndex] = (u8)((r32)At[colorIndex]* Alpha); 
                }
                
                At += 4; 
            }
        }
        
    }
    
    return Result;
    
}

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT(x) Assert(x)
#include "stb_image.h"

#if 1
//#define ExtractBits(Pointer, Shift) (u32)((*Pointer >> Shift) & 0xff)
internal bitmap LoadImage(game_memory *Memory, memory_arena *Arena, char *Filename, v2 AlignPercent) {
    int x,y,n;
    char *data = (char *)stbi_load(Filename, &x, &y, &n, 4);
    
    Assert(x != 0 && y != 0);
    //Assert(n == 4);
    
    bitmap Result = CreateNewBitmap(Arena, x, y, true, V4(1, 0, 1, 1));
    //bitmap Result = LoadBitmap(Memory, Arena, Filename);
    Result.AlignPercent = AlignPercent;
    
    u32 *Src = (u32 *)data;
    u32 *Dest = (u32 *)Result.Bits;
    
    //Result.Bits = data;
    /* We could use SIMD for this (practice?), but probably in the shipping game, this will all be done offline in the asset packer*/
#if 1
    for(s32 i = 0; i < y; ++i){
        for(s32 j = 0; j < x; ++j) {
            u32 B = (u32)((*Src) >> 0 & 0xff);
            u32 G = (u32)((*Src) >> 8 & 0xff);
            u32 R = (u32)((*Src) >> 16 & 0xff);
            u32 A = (u32)((*Src) >> 24 & 0xff);
            
            u32 Color = (A << 24 |
                         R << 0 |
                         G << 8 |
                         B << 16);
            
            //Assert(Color == *Src);
            *Dest = Color;
            Dest++;
            Src++;
        }
    }
    
    Assert(data != Result.Bits);
#endif
    stbi_image_free(data);
    
    
    return Result;
}
#endif 

internal void
DrawBitmap(bitmap *Buffer, bitmap *Bitmap, s32 XOrigin_, s32 YOrigin_, rect2 Bounds, v4 Color = V4(1, 1, 1, 1))
{
    if(Bitmap) {
        
        Bounds = ClipRectangle(Bounds, Rect2(0, 0, (r32)Buffer->Width - 1, (r32)Buffer->Height - 1));
        Bounds = Rect2(0, 0, (r32)Buffer->Width - 1, (r32)Buffer->Height - 1);
        
        /*
        // TODO(OLIVER): This needs fixing for clip space
        Bounds = Rect2(Max(Bounds.Min.X, 0), Max(Bounds.Min.Y, 0), Min(Bounds.Max.X, Buffer->Width - 1), Min(Bounds.Max.Y, Buffer->Height - 1));
        ///////
        */
        r32 Inv_255 = 1.0f / 255.0f;
        
        s32 XOrigin = XOrigin_ - (s32)(Bitmap->AlignPercent.X*Bitmap->Width);
        s32 YOrigin = YOrigin_ - (s32)(Bitmap->AlignPercent.Y*Bitmap->Height);
        
        s32 YMin = 0;
        s32 XMin = 0;
        s32 XMax = (s32)Bitmap->Width;
        s32 YMax = (s32)Bitmap->Height;
        
        s32 XMaxOffset = (XOrigin + XMax) - (s32)Bounds.MaxX;
        s32 YMaxOffset = (YOrigin + YMax) - (s32)Bounds.MaxY;
        if(XMaxOffset >= 0) XMax = XMax - XMaxOffset;
        if(YMaxOffset >= 0) YMax = YMax - YMaxOffset;
        
        if(XOrigin < (s32)Bounds.MinX)
        {
            XMin = (s32)Abs((r32)XOrigin);
            XOrigin = (s32)Bounds.MinX;
        }
        if(YOrigin < (s32)Bounds.MinY)
        {
            YMin = (s32)Abs((r32)YOrigin);
            YOrigin = (s32)Bounds.MinY;
        }
        
        Assert(XMax <= (s32)Bitmap->Width);
        Assert(YMax <= (s32)Bitmap->Height);
        
        u8 *Dest = (u8 *)Buffer->Bits + YOrigin*Buffer->Pitch + XOrigin*sizeof(u32);
        u8 *Source = (u8 *)Bitmap->Bits;
        
        
        for(s32 Y = YMin;
            Y < YMax;
            ++Y)
        {
            
            u32 *DestAt = (u32 *)Dest;
            u32 *SourceAt = (u32 *)Source;
            
            for(s32 X = XMin;
                X < XMax;
                ++X)
            {
                u32 SC = *SourceAt;
                v4 S;
                S.R = (r32)((SC >> 16) & 0xFF);
                S.G = (r32)((SC >> 8) & 0xFF);
                S.B = (r32)((SC >> 0) & 0xFF);
                S.A = (r32)((SC >> 24) & 0xFF);
                
                S = Hadamard(Color, S);
                
                u32 DC = *DestAt;
                v4 D;
                D.R = (r32)((DC >> 16) & 0xFF);
                D.G = (r32)((DC >> 8) & 0xFF);
                D.B = (r32)((DC >> 0) & 0xFF);
                D.A = (r32)((DC >> 24) & 0xFF);
                
                r32 NormalAlpha = Inv_255 * S.A;
                
                r32 BR = D.R + (S.R - D.R)*NormalAlpha;
                r32 BG = D.G + (S.G - D.G)*NormalAlpha;
                r32 BB = D.B + (S.B - D.B)*NormalAlpha;
                r32 BA = D.A + (S.A - D.A)*NormalAlpha;
                
                *DestAt = ((u32)BA << 24) | ((u32)BR << 16) | ((u32)BG << 8) | ((u32)BB << 0);
                //*DestAt = SC; //Blt the source values
                DestAt++;
                SourceAt++;
            }
            
            Dest += Buffer->Pitch;
            Source += Bitmap->Pitch;
        }
    }
}

inline void
LerpToEdge(r32 *Start, r32 *End, r32 Edge)
{
    r32 StartX_ = *Start;
    r32 EndX_ = *End;
    
    if(StartX_ < Edge && EndX_ >= Edge)
    {
        r32 t = 0.0f;
        t = (Edge - StartX_) / (EndX_ - StartX_);
        *Start += t*(EndX_ - StartX_);
    }
}

internal b32
DrawLine(bitmap *Buffer, r32 StartX_, r32 StartY_, r32  EndX_, r32 EndY_, u32 Color)
{
    
    b32 Result = false;
    
    //TODO(ollie): Line Clipping for line -> something to think about
    // SetBitmapValueSafe() handles out of bounds at the moment
#if 0
    LerpToEdge(&StartX_, &EndX_, 0.0f);
    LerpToEdge(&EndX_, &StartX_, 0.0f);
    LerpToEdge(&StartX_, &EndX_, (r32)Buffer->Width);
    LerpToEdge(&EndX_, &StartX_, (r32)Buffer->Width);
    
    LerpToEdge(&StartY_, &EndY_, 0.0f);
    LerpToEdge(&EndY_, &StartY_, 0.0f);
    LerpToEdge(&StartY_, &EndY_, (r32)Buffer->Height);
    LerpToEdge(&EndY_, &StartY_, (r32)Buffer->Height);
    
    StartX_ = Max(StartX_, 0);
    StartX_ = Min(StartX_, Buffer->Width);
    
    EndX_ = Max(EndX_, 0);
    EndX_ = Min(EndX_, Buffer->Width);
    
    StartY_ = Max(StartY_, 0);
    StartY_ = Min(StartY_, Buffer->Height);
    
    EndY_ = Max(EndY_, 0);
    EndY_ = Min(EndY_, Buffer->Height);    
    b32 DrawLine = true;
    
    if(StartY_ < 0.0f)
    {
        Assert(EndY_ < 0.0f);
        DrawLine = false;
    }
    if(StartX_ < 0.0f)
    {
        Assert(EndX_ < 0.0f);
        DrawLine = false;
    }
    if(StartY_ >= Buffer->Height)
    {
        Assert(EndY_ >= Buffer->Height);
        DrawLine = false;
    }
    if(StartX_ >= Buffer->Width)
    {
        Assert(EndX_ >= Buffer->Width);
        DrawLine = false;
    }
#endif
    
    v2 Start = V2(StartX_, StartY_);
    v2 End = V2(EndX_, EndY_);
    if(End.X < Start.X)
    {
        v2 Temp = Start;
        Start = End;
        End = Temp;
    }
    
    v2 UpIncrement = V2(0, 1.0f);
    if(End.Y < Start.Y)
    {
        UpIncrement = V2(0, -1.0f);
    }
    
    v2 NormalLine = Normal(End - Start);
    
    r32 LineLength = Length(End - Start);
    
    for(v2 Pos = Start;
        Inner(NormalLine, Pos - Start) < LineLength;
        )
    {
        Result = false;
        SetBitmapValueSafe(Buffer, (u32)Pos.X, (u32)Pos.Y, Color);
        
        v2 OptionRight = Pos + V2(1.0f, 0);
        v2 OptionUp = Pos + UpIncrement;
        
        r32 RightScore = Abs(Inner(Perp(NormalLine), OptionRight - Start));
        r32 UpScore = Abs(Inner(Perp(NormalLine), OptionUp - Start));
        
        if(RightScore < UpScore)
        {
            Pos = OptionRight;
        }
        else
        {
            Pos = OptionUp;
        }
    }
    
    SetBitmapValueSafe(Buffer, (u32)End.X, (u32)End.Y, Color);
    
    return Result;
}

internal void
DrawRectangle(bitmap *Buffer, rect2 Rect, v4 Color01)
{
    u32 Color255 = Color01AsV4To255AsU32(Color01);
    
    DrawLine(Buffer, GetMinCorner(Rect).X, GetMinCorner(Rect).Y,
             GetMinCorner(Rect).X, GetMaxCorner(Rect).Y, Color255);
    
    DrawLine(Buffer, GetMinCorner(Rect).X, GetMinCorner(Rect).Y,
             GetMaxCorner(Rect).X, GetMinCorner(Rect).Y, Color255);
    
    DrawLine(Buffer, GetMinCorner(Rect).X, GetMaxCorner(Rect).Y,
             GetMaxCorner(Rect).X, GetMaxCorner(Rect).Y, Color255);
    
    DrawLine(Buffer, GetMaxCorner(Rect).X, GetMinCorner(Rect).Y,
             GetMaxCorner(Rect).X, GetMaxCorner(Rect).Y, Color255);
}

inline v2 Scale(transform *Transform, v2 Pos) {
    v2 Result = Hadamard(Transform->Scale, Pos);
    return Result;
}

inline v2 Transform(transform *Transform, v2 Pos) {
    v2 Result = Hadamard(Transform->Scale, Pos); // SCALE
    Result = Mat2MultiplyV2(Result, Transform->Rotation);
    Result += Transform->Offset; //TRANSLATE
    return Result;
}

inline rect2 Transform(transform *TransformPtr, rect2 Dim) {
    rect2 Result = Rect2MinMax(Transform(TransformPtr, Dim.Min), Transform(TransformPtr, Dim.Max)); 
    return Result;
}

inline v2 InverseTransform(transform *Transform, v2 Pos) {
    
    v2 Result = Pos - Transform->Offset;
    Result = Mat2MultiplyV2(Result, Transpose(Transform->Rotation));
    Result = Hadamard(Result, V2(1.0f / Transform->Scale.X, 1.0f / Transform->Scale.Y));
    
    return Result;
} 

void PushClear(render_group *Group, v4 Color) {
    render_element_header *Header = (render_element_header *)PushSize(&Group->Arena, sizeof(render_element_clear) + sizeof(render_element_header));
    
    Header->Type = render_clear;
    render_element_clear *Info = (render_element_clear *)(Header + 1);
    Info->Color = Color;
}

void PushRectOutline(render_group *Group, rect2 Dim, r32 ZDepth, v4 Color = V4(0, 0, 0, 1)) {
    render_element_header *Header = (render_element_header *)PushSize(&Group->Arena, sizeof(render_element_rect_outline) + sizeof(render_element_header));
    
    Header->Type = render_rect_outline;
    render_element_rect_outline *Info = (render_element_rect_outline *)(Header + 1);
    
    Info->ZDepth = ZDepth;
    Info->Dim = Transform(&Group->Transform, Dim);
    Info->Color = Color;
}
inline void PushRectCenterOutline(render_group *Group, rect2 Dim, r32 ZDepth, v4 Color = V4(0, 0, 0, 1)) {
    
    v2 WHDim = V2(GetWidth(Dim), GetHeight(Dim));
    rect2 NewDim = Rect2CenterDim(Dim.Min, WHDim);
    PushRectOutline(Group, NewDim, ZDepth, Color);
    
}
//THREAD_WORK_FUNCTION(OpenGlLoadTextureThreadWork);
void PushBitmap(render_group *Group, v3 Pos, bitmap *Bitmap, r32 WidthInWorldSpace,  rect2 ClipRect, v4 Color = V4(1, 1, 1, 1)) {
    
    switch(Bitmap->LoadState) {
        case RESOURCE_NOT_LOADED: {
            Assert(!Bitmap->Handle);
            //We are the only ones asking for memory arena. Other threads can release them but can't use them. 
            game_state *GameState = Group->GameState;
            memory_arena *Arena = GetThreadMemoryArena(GameState);
            
            if(Arena) {
                Bitmap->LoadState = RESOURCE_LOADING;
#define LOAD_TEXTURE_ON_SEPERATE_THREAD 1
#if LOAD_TEXTURE_ON_SEPERATE_THREAD
                temp_memory TempMem = MarkMemory(Arena);
                
                render_load_texture_info *Data = PushStruct(Arena, render_load_texture_info);
                
                Data->Bitmap = Bitmap;
                Data->MemoryMark = TempMem;
                
                Group->Memory->PlatformPushWorkOntoQueue(Group->ThreadInfo, OpenGlLoadTextureThreadWork, Data);
#else 
                OpenGlLoadTexture(Bitmap, ++GameState->TextureHandleIndex);
                Bitmap->LoadState = RESOURCE_LOADED;
#endif
            }
        } break;
        case RESOURCE_LOADING: {
            //do nothing
        } break;
        case RESOURCE_LOADED: {
            Assert(Bitmap->LoadState == RESOURCE_LOADED);
            render_element_header *Header = (render_element_header *)PushSize(&Group->Arena, sizeof(render_element_bitmap) + sizeof(render_element_header));
            
            
            Header->Type = render_bitmap;
            render_element_bitmap *Info = (render_element_bitmap *)(Header + 1);
            
            Info->Bitmap = Bitmap;
            Info->ClipRect = ClipRect;
            Info->Color = Color;
            r32 WidthToHeightRatio = SafeRatio0((r32)Bitmap->Height, (r32)Bitmap->Width); 
            r32 HeightInWorldSpace = WidthToHeightRatio*WidthInWorldSpace;
            Info->Dim = Scale(&Group->Transform, V2(WidthInWorldSpace, HeightInWorldSpace));
            Info->Pos = Transform(&Group->Transform, Pos.XY) - Hadamard(Bitmap->AlignPercent, Info->Dim);
            Info->ZDepth = Pos.Z;
        } break;
        default: {
            InvalidCodePath;
        }
    }
}


void PushBitmapScale(render_group *Group, v3 Pos, bitmap *Bitmap, r32 Scale,  rect2 ClipRect, v4 Color = V4(1, 1, 1, 1)) {
    
    PushBitmap(Group, Pos, Bitmap, Scale*Bitmap->Width, ClipRect, Color);
}
void PushRect(render_group *Group, rect2 Dim, r32 ZDepth, v4 Color) {
    render_element_header *Header = (render_element_header *)PushSize(&Group->Arena, sizeof(render_element_rect) + sizeof(render_element_header));
    
    Header->Type = render_rect;
    render_element_rect *Info = (render_element_rect *)(Header + 1);
    
    Info->ZDepth = ZDepth;
    Info->Dim = Transform(&Group->Transform, Dim);
    Info->Color = Color;
}

void PushRectCenter(render_group *Group, rect2 Dim, r32 ZDepth, v4 Color) {
    v2 WHDim = V2(GetWidth(Dim), GetHeight(Dim));
    rect2 NewDim = Rect2CenterDim(Dim.Min, WHDim);
    PushRect(Group, NewDim, ZDepth, Color);
}


internal void InitRenderGroup(render_group *Group, bitmap *Buffer,memory_arena *MemoryArena, memory_size MemorySize, v2 Scale, v2 Offset, mat2 Rotation, thread_info *ThreadInfo, game_state *State, game_memory *Memory) {
    *Group = {};
    Group->ScreenDim = V2i(Buffer->Width, Buffer->Height);
    Group->Buffer = Buffer;
    Group->TempMem = MarkMemory(MemoryArena);
    Group->Arena = SubMemoryArena(MemoryArena, MemorySize);
    Group->Transform.Scale = Scale; 
    Group->Transform.Offset = Offset;
    Group->Transform.Rotation = Rotation;
    Group->ThreadInfo = ThreadInfo;
    Group->GameState = State;
    Group->Memory = Memory;
}

void RenderGroupToOutput(render_group *Group) {
    u8 *Base = (u8 *)Group->Arena.Base;
    u8 *At = Base;
    
    rect2 BufferRect = Rect2(0, 0, Group->ScreenDim.X, Group->ScreenDim.Y);
    while((s32)(At - Base) < Group->Arena.CurrentSize) {
        render_element_header *Header = (render_element_header *)At;
        switch(Header->Type) {
            case render_clear: {
                render_element_clear *Info = (render_element_clear *)(Header + 1);
                
                ClearBitmap(Group->Buffer, Info->Color);
                
                At += sizeof(render_element_header) + sizeof(render_element_clear);
            } break;
            case render_bitmap: {
                render_element_bitmap *Info = (render_element_bitmap *)(Header + 1);
                
                DrawBitmap(Group->Buffer, Info->Bitmap, (s32)Info->Pos.X, (s32)Info->Pos.Y, Info->ClipRect, Info->Color);
                
                At += sizeof(render_element_header) + sizeof(render_element_bitmap);
            } break;
            case render_rect: {
                render_element_rect *Info = (render_element_rect *)(Header + 1);
                
                FillRectangle(Group->Buffer, Info->Dim, Info->Color);
                
                At += sizeof(render_element_header) + sizeof(render_element_rect);
            } break;
            case render_rect_outline: {
                render_element_rect_outline *Info = (render_element_rect_outline *)(Header + 1);
                
                DrawRectangle(Group->Buffer, Info->Dim, Info->Color);
                
                At += sizeof(render_element_header) + sizeof(render_element_rect_outline);
            } break;
            default: {
                InvalidCodePath;
            }
        }
        Assert((At - Base) >= 0);
    }
    
    ReleaseMemory(&Group->TempMem);
}

#if !defined(CALM_GAME_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

struct memory_arena
{
    void *Base;
    size_t CurrentSize;
    size_t TotalSize;
    
    u32 TempMemCount;
};

struct temp_memory
{
    memory_arena *Arena;
    size_t OriginalSize;
    
    u32 ID;
};

#define PushArray(Arena, Type, Size, ...) (Type *)PushSize(Arena, Size*sizeof(Type), __VA_ARGS__)
#define PushStruct(Arena, Type) (Type *)PushSize(Arena, sizeof(Type))


inline void *
PushSize(memory_arena *Arena, size_t Size, b32 Clear = true)
{
    Assert((Arena->CurrentSize + Size) <= Arena->TotalSize);
    void *Result = (void *)((u8 *)Arena->Base + Arena->CurrentSize);
    Arena->CurrentSize += Size;
    
    if(Clear)
    {
        size_t ClearSize = Size;
        u8 *Memory = (u8 *)Result; 
        while(ClearSize--)
        {
            *Memory++ = 0;
        }
    }    
    return Result;
    
}

inline void
MemoryCopy(void *Source, void *Dest, u32 Size)
{
    u8 *Source_u8 = (u8 *)Source;
    u8 *Dest_u8 = (u8 *)Dest;
    
    while(Size--)
    {
        *Dest_u8++ = *Source_u8++;
        
    }
}


inline void InitializeMemoryArena(memory_arena *Arena, void *Memory, size_t Size) {
    Arena->Base = (u8 *)Memory;
    Arena->CurrentSize = 0;
    Arena->TotalSize = Size;
} 

inline memory_arena SubMemoryArena(memory_arena *Arena, size_t Size) {
    memory_arena Result = {};
    
    Result.TotalSize = Size;
    Result.CurrentSize = 0;
    Result.Base = (u8 *)PushSize(Arena, Result.TotalSize);
    
    return Result;
}


#define CopyArray(Source, Dest, Type, Count) MemoryCopy(Source, Dest, sizeof(Type)*Count);


inline b32
DoStringsMatch(char *A, char *B)
{
    b32 Result = true;
    while(*A && *B)
    {
        Result &= (*A++ == *B++);
    }
    
    Result &= (!*A && !*B);
    
    return Result;
}

inline temp_memory
MarkMemory(memory_arena *Arena)
{
    temp_memory Result = {};
    Result.Arena = Arena;
    Result.OriginalSize = Arena->CurrentSize;
    Result.ID = Arena->TempMemCount++;
    
    return Result;
    
}

inline void
ReleaseMemory(temp_memory *TempMem)
{
    TempMem->Arena->CurrentSize = TempMem->OriginalSize;
    u32 ID = --TempMem->Arena->TempMemCount;
    Assert(TempMem->ID == ID);
    
}

#define MoveToList(LinkPtr, FreeList, type) type *Next = (*LinkPtr)->Next; (*LinkPtr)->Next = FreeList; FreeList = *LinkPtr; *LinkPtr = Next;

#include "calm_bucket.cpp"
#include "calm_math.h"
#include "calm_random.h"
#include "calm_font.h"
#include "calm_console.h"
#include "calm_sound.h"
#include "calm_entity.h"
#include "calm_render.h"

enum game_mode {
    PLAY_MODE,
    MENU_MODE,
};

struct quality_info {
    r32 MaxValue;
    b32 IsPeriodic;
};

enum animation_qualities {
    POSE,
    DIRECTION,
    ANIMATE_QUALITY_COUNT
};

struct animation {
    bitmap Frames[32];
    u32 FrameCount;
    r32 Qualities[ANIMATE_QUALITY_COUNT];
};

static r32 WorldChunkInMeters = 0.7f;

struct world_chunk {
    s32 X;
    s32 Y;
    
    world_chunk *Next;
};

#define WORLD_CHUNK_HASH_SIZE 4096

struct game_state
{
    memory_arena MemoryArena;
    memory_arena PerFrameArena;
    memory_arena SubArena;
    memory_arena ScratchPad;
    
    playing_sound *PlayingSounds;
    playing_sound *PlayingSoundsFreeList;
    
    entity *Camera;
    
    loaded_sound BackgroundMusic;
    
    search_cell *SearchCellFreeList;
    search_cell SearchCellSentinel;
    
    entity *Player;
    
    b32 RenderConsole;
    
    game_mode GameMode;
    s32 MenuIndex;
    
    world_chunk *Chunks[WORLD_CHUNK_HASH_SIZE]; 
    
    entity *InteractingWith;
    entity *HotEntity;
    
    
    u32 EntityCount;
    entity Entities[64];
    
    font *Fonts;
    font *Font;
    bitmap MossBlockBitmap;
    bitmap MagicianHandBitmap;
    
    animation *CurrentAnimation;
    u32 FrameIndex;
    r32 FramePeriod;
    r32 FrameTime;
    
    r32 MenuTime;
    r32 MenuPeriod;
    
    u32 LanternAnimationCount;
    animation LanternManAnimations[16];
    
    random_series GeneralEntropy;
    
    b32 IsInitialized;
    
};

inline b32
InBounds(bitmap *Bitmap, u32 X, u32 Y)
{
    b32 Result = ((X >= 0) && (X < Bitmap->Width) && (Y >= 0) && (Y < Bitmap->Height));
    return Result;
}

inline b32
InBounds(u32 X, u32 Y, u32 Width, u32 Height)
{
    b32 Result = ((X >= 0) && (X < Width) && (Y >= 0) && (Y < Height));
    return Result;
}

inline void
SetBitmapValueSafe(bitmap *Bitmap, u32 X, u32 Y, u32 Value)
{
    if(InBounds(Bitmap, X, Y))
    {
        ((u32 *)Bitmap->Bits)[Y*Bitmap->Width + X] = Value;
    }
}

inline void
SetBitmapValue(bitmap *Bitmap, u32 X, u32 Y, u32 Value)
{
    ((u32 *)Bitmap->Bits)[Y*Bitmap->Width + X] = Value;
}

inline u32
GetValue(bitmap *Bitmap, u32 X, u32 Y)
{
    Assert(InBounds(Bitmap, X, Y));
    u32 Result = (((u32 *)Bitmap->Bits)[Y*Bitmap->Width + X]);
    return Result;
}

#define CALM_GAME_H
#endif

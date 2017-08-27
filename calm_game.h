#if !defined(CALM_GAME_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

////////////////Gameplay defines/////////
#define END_WITH_OFFSET 0
#define MOVE_DIAGONAL 0
#define MOVE_VIA_MOUSE 0
#define PLAYER_MOVE_ACTION IsDown
#define DEFINE_MOVE_ACTION WasPressed
#define DRAW_PLAYER_PATH 0
#define CREATE_PHILOSOPHER 1
#define UPDATE_CAMERA_POS 1
/////////////////////////////////////////


#define CopyArray(Source, Dest, Type, Count) MemoryCopy(Source, Dest, sizeof(Type)*Count);

//Can we make this into a more generalised funciton? Maybe with meta-programming?
inline b32 InArray(u32 *Array, u32 ArrayCount, u32 Value) {
    b32 Result = false;
    forN(ArrayCount) {
        if(Array[ArrayCountIndex] == Value) {
            Result = true;
            break;
        }
    }
    return Result;
}


internal u32 StringLength(char *A) {
    u32 Result = 0;
    
    while (*A++)
    {
        Result++;
    }
    
    return Result;
}

#define MoveToList(LinkPtr, FreeList, type) type *Next = (*LinkPtr)->Next; (*LinkPtr)->Next = FreeList; FreeList = *LinkPtr; *LinkPtr = Next;

struct timer {
    r32 Value;
    r32 Period;
};

inline b32 UpdateTimer(timer *Timer, r32 dt, r32 ResetValue = 0.0f) {
    Timer->Value += dt;
    b32 WasReset = false;
    if((Timer->Value / Timer->Period) >= 1.0f) {
        Timer->Value = ResetValue;
        WasReset = true;
    }
    Assert((Timer->Value/Timer->Period) <= 1.0f);
    return WasReset;
}

inline r32 CanonicalValue(timer *Timer) {
    r32 Result = Timer->Value/Timer->Period;
    return Result;
}

enum chunk_type {
    ChunkNull, 
    ChunkLight,
    ChunkDark,
    ChunkBlock,
    ChunkMain,
    
    ChunkTypeCount
};

#include "calm_bucket.cpp"
#include "calm_random.h"
#include "calm_console.h"
#include "handmade_audio.h"
#include "calm_entity.h"
#include "calm_menu.h"


enum game_mode {
    PLAY_MODE,
    GAMEOVER_MODE,
    WIN_MODE,
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

static r32 WorldChunkInMeters = 1.0f;

struct world_chunk {
    s32 X;
    s32 Y;
    
    chunk_type Type;
    chunk_type MainType;
    
    world_chunk *Next;
};

#define WORLD_CHUNK_HASH_SIZE 4096

struct ui_state;
struct game_state
{
    memory_arena MemoryArena;
    memory_arena PerFrameArena;
    memory_arena SubArena;
    memory_arena ScratchPad;
    memory_arena RenderArena;
    
    playing_sound *PlayingSounds;
    playing_sound *PlayingSoundsFreeList;
    
    //These should never move i.e. we should never get a null pointer. Maybe we could change all entity pointers to an ID and we look up that ID instead of getting dangling pointer...//
    entity *Camera;
    entity *Player;
    
    loaded_sound BackgroundMusic;
    loaded_sound FootstepsSound[32];
    
    search_cell *SearchCellFreeList;
    search_cell SearchCellSentinel;
    
    //TODO: Can we get rid of this using z-indexes!
    b32 RenderConsole;
    //
    
    game_mode GameMode;
    
    world_chunk *Chunks[WORLD_CHUNK_HASH_SIZE]; 
    
    u32 EntityCount;
    entity Entities[64];
    
    font *Fonts;
    font *GameFont;
    font *DebugFont;
    bitmap MossBlockBitmap;
    bitmap MagicianHandBitmap;
    
    animation *CurrentAnimation;
    u32 FrameIndex;
    r32 FramePeriod;
    r32 FrameTime;
    
    menu PauseMenu;
    menu GameOverMenu;
    
    u32 LanternAnimationCount;
    animation LanternManAnimations[16];
    
    random_series GeneralEntropy;
    
    //
    b32 PlayerIsSettingPath;
    path_nodes PathToSet;
    //
    
    ui_state *UIState;
    
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

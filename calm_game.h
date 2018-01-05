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
#define CREATE_PHILOSOPHER 0
#define UPDATE_CAMERA_POS 1
#define DELETE_PHILOSOPHER_ON_CAPTURE 0
#define LOAD_LEVEL_FROM_FILE 0
/////////////////////////////////////////


#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &Instance)
#define ZeroArray(Instance, Size) ZeroSize(Size*sizeof((Instance)[0]), Instance)

inline void
ZeroSize(memory_size Size, void *Ptr){
    uint8 *Byte = (uint8 *)Ptr;
    while(Size--)
    {
        *Byte++ = 0;
    }
}

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


#define MoveToList(LinkPtr, FreeList, type) type *Next = (*LinkPtr)->Next; (*LinkPtr)->Next = FreeList; FreeList = *LinkPtr; *LinkPtr = Next;

#define GetStringSizeFromChar(At, Start) (u32)((intptr)At - (intptr)Start)
#define GetRemainingSize(At, Start, TotalSize) (TotalSize - GetStringSizeFromChar(At, Start))

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
    ChunkEntity,
    ChunkMain,
    
    ChunkTypeCount
};

#include "calm_bucket.cpp"
#include "calm_random.h"
#include "calm_console.h"
#include "calm_audio.h"
#include "calm_particles.h"
#include "calm_animation.h"
#include "calm_world_chunk.h"
#include "calm_entity.h"
#include "calm_menu.h"


enum game_mode {
    PLAY_MODE,
    GAMEOVER_MODE,
    WIN_MODE,
    MENU_MODE,
};

static r32 WorldChunkInMeters = 1.0f;

enum tile_pos_type {
    NULL_TILE,
    
    TOP_LEFT_TILE,
    TOP_CENTER_TILE,
    TOP_RIGHT_TILE,
    
    CENTER_LEFT_TILE,
    CENTER_TILE,
    CENTER_RIGHT_TILE,
    
    BOTTOM_LEFT_TILE,
    BOTTOM_CENTER_TILE,
    BOTTOM_RIGHT_TILE,
    
    TILE_POS_COUNT
};

struct tile_type_layout {
    tile_pos_type Type;
    s32 E[3][3];
};


struct ui_state;
struct game_state
{
    memory_arena MemoryArena;
    memory_arena PerFrameArena;
    memory_arena ScratchPad;
    memory_arena RenderArena;
    memory_arena StringArena;
    memory_arena ThreadArenas[16];
    memory_arena TransientArena;
    
    audio_state AudioState;
    
    playing_sound *PlayingSounds;
    playing_sound *PlayingSoundsFreeList;
    
    animation_list_item *AnimationItemFreeList;
    
    //These should never move i.e. we should never get a null pointer. Maybe we could change all entity pointers to an ID and we look up that ID instead of getting dangling pointer...//
    //DONE!//
    
    u32 TextureHandleIndex;
    
    loaded_sound OpenSound;
    loaded_sound MenuBackgroundMusic;
    loaded_sound BackgroundMusic;
    
    u32 PushSoundCount;
    loaded_sound PushSound[4];
    loaded_sound FootstepsSound[32];
    
    playing_sound *BackgroundSoundInstance;
    playing_sound *MenuModeSoundInstance;
    
    world_chunk *ChunkFreeList;
    
    search_cell *SearchCellFreeList;
    search_cell SearchCellSentinel;
    
    //TODO: Can we get rid of this using z-indexes!
    b32 RenderConsole;
    //
    
    game_mode GameMode;
    
    u32 EntityIDAt; // 0 index is a null ID
    u32 EntityCount;
    entity Entities[64];
    
    world_chunk *Chunks[WORLD_CHUNK_HASH_SIZE]; 
    
    font *Fonts;
    font *GameFont;
    font *DebugFont;
    bitmap MossBlockBitmap;
    bitmap MagicianHandBitmap;
    
    bitmap DarkTiles[TILE_POS_COUNT];
    
    bitmap StaticUp;
    bitmap StaticDown;
    bitmap StaticLeft;
    bitmap StaticRight;
    
    bitmap Leaves;
    bitmap Floor;
    bitmap Fog;
    bitmap Man;
    bitmap Shadow;
    bitmap Arms;
    bitmap Legs;
    bitmap FootPrint;
    bitmap Desert;
    bitmap Water;
    bitmap Crate;
    bitmap Door;
    bitmap Monster;
    
    bitmap BackgroundImage;
    
    v2 RestartPlayerPosition;
    
    u32 FootPrintCount;
    u32 FootPrintIndex;
    v2 FootPrintPostions[1028];
    
    b32 HideUnderWorld;
    timer AlphaFadeTimer;
    
    menu PauseMenu;
    menu GameOverMenu;
    
    u32 KnightAnimationCount;
    animation KnightAnimations[16];
    
    u32 ManAnimationCount;
    animation ManAnimations[16];
    
    random_series GeneralEntropy;
    
    b32 ShowHUD;
    b32 RenderMainChunkType;
    timer SettingPathTimer;
    
    //
    b32 PlayerIsSettingPath;
    path_nodes PathToSet;
    //
    
    ui_state *UIState;
    
    b32 IsInitialized;
    
};

inline memory_arena *GetThreadMemoryArena(game_state *GameState) {
    memory_arena *Result = 0;
    
    for(u32 i = 0; i < ArrayCount(GameState->ThreadArenas); ++i) {
        memory_arena *Arena = GameState->ThreadArenas + i;
        if(Arena->TempMemCount == 0) {
            Result = Arena;
        }
    }
    
    return Result;
} 

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

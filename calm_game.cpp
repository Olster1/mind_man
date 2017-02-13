/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#include "calm_game.h"
#include "calm_sound.cpp"
#include "calm_render.cpp"
#include "calm_menu.cpp"
#include "calm_print.cpp"
#include "calm_console.cpp"
#include "calm_entity.cpp"

internal void InitAnimation(animation *Animation, game_memory *Memory,  char **FileNames, u32 FileNameCount, r32 DirectionValue, r32 PoseValue) {
    Animation->Qualities[POSE] = PoseValue;
    Animation->Qualities[DIRECTION] = DirectionValue;
    
    for(u32 i = 0; i < FileNameCount; ++i) {
        Animation->Frames[Animation->FrameCount++] = LoadBitmap(Memory, 0, FileNames[i]);
        
    }
}

static quality_info QualityInfo[ANIMATE_QUALITY_COUNT] = {{1.0, false}, {2*PI32, true}};

internal animation *GetAnimation(animation *Animations, u32 AnimationsCount,  r32 *Qualities,
                                 r32 *Weights) {
    animation *BestResult = 0;
    r32 BestValue = 100000.0f;
    forN(AnimationsCount) {
        animation *Anim = Animations + AnimationsCountIndex;
        r32 ThisValue = 0;
        for(u32 i = 0; i < ANIMATE_QUALITY_COUNT; ++i) {
            r32 Value = Qualities[i];
            r32 TestValue = Anim->Qualities[i];
            quality_info Info= QualityInfo[i];
            
            r32 DifferenceSqr = Sqr(TestValue - Value);
            if(Info.IsPeriodic) {
                if(DifferenceSqr > Sqr(Info.MaxValue/2)) {
                    r32 Min = TestValue;
                    r32 Max = Value;
                    if(Max < Min) {
                        Min = Value;
                        Max = TestValue;
                    }
                    Min += Info.MaxValue/2;
                    DifferenceSqr = Sqr(Max - Min);
                }
            }
            ThisValue += Weights[i]*(DifferenceSqr / Info.MaxValue);
        }
        if(ThisValue < BestValue) {
            BestResult = Anim;
            BestValue = ThisValue;
        }
    }
    
    return BestResult;
}

struct search_cell {
    u32 X; 
    u32 Y;
    
    search_cell *Prev;
    search_cell *Next;
};

inline void PushOnToList(search_cell **SentinelPtr, u32 X, u32 Y, memory_arena *Arena, search_cell **FreelistPtr) {
    search_cell *Sentinel = *SentinelPtr;
    
    search_cell *NewCell = *FreelistPtr;
    
    if(NewCell) {
        *FreelistPtr = (*FreelistPtr)->Next;
    } else {
        NewCell = PushStruct(Arena, search_cell);
    }
    
    NewCell->X = X;
    NewCell->Y = Y;
    
    NewCell->Next = Sentinel->Next;
    NewCell->Prev = Sentinel;
    
    Sentinel->Next->Prev = NewCell;
    Sentinel->Next = NewCell;
}

inline void RemoveOffList(search_cell **SentinelPtr, search_cell *CellToRemove, search_cell **FreelistPtr) {
    search_cell *Sentinel = *SentinelPtr;
    
    Assert(Sentinel->Prev == CellToRemove);
    Sentinel->Prev->Prev->Next = Sentinel;
    Sentinel->Prev = Sentinel->Prev->Prev;
    
    CellToRemove->Next = *FreelistPtr;
    *FreelistPtr = CellToRemove;
    
}

internal void UpdatePieceOfEntityPosViaFunction(entity *Entity, r32 dt) {
    Entity->Pos = Lerp(Entity->StartPos, Entity->MoveT / Entity->MovePeriod, Entity->TempTargetPos);
    
    v2 Direction = Normal(Entity->TargetPos - Entity->Pos);
    Entity->Velocity = 20.0f*Direction;
    Entity->MoveT += dt;
    if(Entity->MoveT / Entity->MovePeriod >= 1.0f) {
        Entity->Pos = Entity->TempTargetPos;
        Entity->Moving = false;
        Entity->MoveT = 0;
        
        AddToOutBuffer("Temp: [%f: %f]\n", Entity->TempTargetPos.X, Entity->TempTargetPos.Y);
        AddToOutBuffer("Target: [%f: %f]\n", Entity->TargetPos.X, Entity->TargetPos.Y);
        if(Entity->TempTargetPos == Entity->TargetPos) {
            Entity->Velocity = {};
            
            
        }
    }
}

internal void UpdateEntityPosViaFunction(entity *Entity, r32 dt) {
    Entity->Pos = SineousLerp0To1(Entity->StartPos, Entity->MoveT / Entity->MovePeriod, Entity->TargetPos);
    
    r32 TValue = Length(Entity->Pos - Entity->StartPos) / Length(Entity->TargetPos - Entity->StartPos); 
    
    v2 Direction = Normal(Entity->TargetPos - Entity->Pos);
    r32 VelFactor = SineousLerp0To0(0, TValue, 200);
    r32 CurrentVelFactor = Length(Entity->Velocity);
    if(VelFactor < CurrentVelFactor) {
        VelFactor = CurrentVelFactor;
    }
    Entity->Velocity = VelFactor*Direction;
    Entity->MoveT += dt;
    if(Entity->MoveT / Entity->MovePeriod >= 1.0f) {
        Entity->Pos = Entity->TargetPos;
        Entity->Velocity = {};
        Entity->Moving = false;
    }
}

inline b32 PushToList(game_state *GameState, s32 X, s32 Y, b32 *VisitedArray, search_cell **SentinelPtr, search_cell **SearchCellFreelistPtr, v2i TargetP) {
    b32 Found = false;
    if(X >= 0 && Y >= 0 && X < WORLD_GRID_SIZE && Y < WORLD_GRID_SIZE && !(*(VisitedArray + Y*WORLD_GRID_SIZE + X)) && !GameState->WorldGrid[Y*WORLD_GRID_SIZE + X]) {   
        
        *(VisitedArray + Y*WORLD_GRID_SIZE + X) = true;
        if(X == TargetP.X && Y == TargetP.Y) { 
            Found = true;
        } else {  
            PushOnToList(SentinelPtr, X, Y, &GameState->ScratchPad, SearchCellFreelistPtr); 
        } 
    }
    return Found;
}

internal void
GameUpdateAndRender(bitmap *Buffer, game_memory *Memory, r32 dt)
{
    game_state *GameState = (game_state *)Memory->GameStorage;
    
    if(!GameState->IsInitialized)
    {
        
        InitializeMemoryArena(&GameState->MemoryArena, (u8 *)Memory->GameStorage + sizeof(game_state), Memory->GameStorageSize - sizeof(game_state));
        
        GameState->PerFrameArena = SubMemoryArena(&GameState->MemoryArena, MegaBytes(2));
        
        GameState->ScratchPad = SubMemoryArena(&GameState->MemoryArena, MegaBytes(2));
        
        GameState->Player = InitEntity(GameState, V2(100, 100), V2(25, 50), Entity_Player);
        
        GameState->Camera = InitEntity(GameState, V2(0, 0), V2(0, 0), Entity_Camera);
        
        InitEntity(GameState, V2(200, 200), V2(50, 50), Entity_Object);
        
        GameState->BackgroundMusic = LoadWavFileDEBUG(Memory, "Moonlight_Hall.wav", &GameState->MemoryArena);
        PushSound(GameState, &GameState->BackgroundMusic, 1.0, true);
        
        GameState->MossBlockBitmap = LoadBitmap(Memory, 0, "moss_block1.bmp");
        GameState->MagicianHandBitmap = LoadBitmap(Memory, 0, "magician_hand.bmp");
        
        GameState->LanternAnimationCount = 0;
        
        char *FileNames[16] = {"lantern_man/lm_run_left_l.bmp",
            "lantern_man/lm_run_left_r.bmp"};
        
        InitAnimation(&GameState->LanternManAnimations[GameState->LanternAnimationCount++], Memory, FileNames, 2, PI32, -0.3);
        
        FileNames[0] = "lantern_man/lm_stand_left.bmp";
        
        InitAnimation(&GameState->LanternManAnimations[GameState->LanternAnimationCount++], Memory, FileNames, 1, PI32, -0.01 );
        
        FileNames[0] = "lantern_man/lm_run_right_l.bmp";
        FileNames[1] = "lantern_man/lm_run_right_r.bmp";
        
        InitAnimation(&GameState->LanternManAnimations[GameState->LanternAnimationCount++], Memory, FileNames, 2, 0, 0.3);
        
        FileNames[0] = "lantern_man/lm_stand_right.bmp";
        
        InitAnimation(&GameState->LanternManAnimations[GameState->LanternAnimationCount++], Memory, FileNames, 1, 0, 0.01);
        
        FileNames[0] = "lantern_man/lm_front.bmp";
        
        InitAnimation(&GameState->LanternManAnimations[GameState->LanternAnimationCount++], Memory, FileNames, 1, PI32*1.5f, 0);
        
        GameState->GameMode = PLAY_MODE;
        
        GameState->Fonts = LoadFontFile("fonts.clm", Memory, &GameState->MemoryArena);
        
        r32 Qualities[FontQuality_Count] = {};
        Qualities[FontQuality_Debug] = 0.0f;
        Qualities[FontQuality_Size] = 1.0f;
        
        
        GameState->Font = FindFont(GameState->Fonts, Qualities);
        InitConsole(&DebugConsole, GameState->Font, Buffer);
        
        GameState->MenuPeriod = 2.0f;
        
        GameState->FramePeriod = 0.2f;
        GameState->FrameIndex = 0;
        GameState->FrameTime = 0;
        GameState->CurrentAnimation = &GameState->LanternManAnimations[0];
        
        GameState->IsInitialized = true;
    }
    
    temp_memory PerFrameMemory = MarkMemory(&GameState->PerFrameArena);
    
    ClearBitmap(Buffer, V4(1, 1, 0, 1));
    
    r32 MetersToPixels = 60.0f;
    
    render_group RenderGroup = {};
    RenderGroup.MetersToPixels = 60.0f;
    RenderGroup.ScreenDim = V2i(Buffer->Width, Buffer->Height);
    
    entity *Player = GameState->Player;
    
    switch(GameState->GameMode) {
        case MENU_MODE: {
            
            UpdateMenu(GameState, Memory, Buffer, dt);
            
        } break;
        case PLAY_MODE: {
            v2 CamPos = GameState->Camera->Pos;
            v2 MouseP = V2(Memory->MouseX, Memory->MouseY);
            v2 MouseP_CamRel = MouseP + CamPos;
            if(WasPressed(Memory->GameButtons[Button_Escape]))
            {
                GameState->GameMode = MENU_MODE;
                break;
            }
            
            v2 Acceleration = {};
            r32 AbsAcceleration = 700.0f;
            if(IsDown(Memory->GameButtons[Button_Left]))
            {
                Acceleration.X = -AbsAcceleration;
            }
            if(IsDown(Memory->GameButtons[Button_Right]))
            {
                Acceleration.X = AbsAcceleration;
            }
            
            if(IsDown(Memory->GameButtons[Button_Up]))
            {
                
            }
            if(IsDown(Memory->GameButtons[Button_Down]))
            {
                
            }
            if(WasPressed(Memory->GameButtons[Button_LeftMouse]))
            {
                if(IsDown(Memory->GameButtons[Button_Shift])) {
                    s32 CellX = (s32)(MouseP.X / WorldChunkInMeters);
                    s32 CellY = (s32)(MouseP.Y / WorldChunkInMeters);
                    
                    u32 Index = CellY*WORLD_GRID_SIZE + CellX;
                    GameState->WorldGrid[Index] = !GameState->WorldGrid[Index];
                    
                } else {
                    entity *NextHotEntity = 0;
                    for(u32 i = 0; i < GameState->EntityCount; ++i) {
                        entity *Entity = GameState->Entities + i;
                        if(Entity->Type == Entity_Object) {
                            
                            v2 EntityRelP = Entity->Pos - CamPos;
                            
                            rect2 EntityAsRect = Rect2CenterDim(EntityRelP, Entity->Dim);
                            if(InBounds(EntityAsRect, MouseP)) {
                                NextHotEntity = Entity;
                            }
                        }
                    }
                    
                    if(!GameState->InteractingWith) {
                        if(!NextHotEntity) {
                            v2 TargetP = MouseP + GameState->Camera->Pos;
                            Player->TargetPos =WorldChunkInMeters*V2i((s32)(TargetP.X / WorldChunkInMeters), (s32)(TargetP.Y / WorldChunkInMeters)); 
                            Player->OffsetTargetP = TargetP - Player->TargetPos;
                            Player->StartPos = Player->Pos;
                            Player->MoveT = 0;
                        } else {
                            GameState->HotEntity = NextHotEntity;
                            GameState->InteractingWith = GameState->HotEntity;
                        }
                    }
                }
            }
            if(WasReleased(Memory->GameButtons[Button_LeftMouse])) {
                
                GameState->InteractingWith = 0;
            }
            
            if(GameState->InteractingWith) {
                GameState->InteractingWith->Pos = MouseP + CamPos;
                
            }
            
            r32 MaxVelocity = 60.0f;
            r32 Qualities[ANIMATE_QUALITY_COUNT] = {};
            r32 Weights[ANIMATE_QUALITY_COUNT] = {10000.0f, 1.0f};
            Qualities[POSE] = Player->Velocity.X / MaxVelocity;
            Qualities[DIRECTION] = atan2(Player->Velocity.Y, Player->Velocity.X); 
            
            animation *NewAnimation = GetAnimation(GameState->LanternManAnimations, GameState->LanternAnimationCount, Qualities, Weights);
            
            r32 FastFactor = 1000;
            GameState->FramePeriod = 0.2f; //FastFactor / LengthSqr(Player->Velocity);
            // TODO(OLIVER): Make this into a sineous function
            GameState->FramePeriod = Clamp(0.2f, GameState->FramePeriod, 0.4f);
            
            if(NewAnimation != GameState->CurrentAnimation) {
                GameState->FrameIndex = 0;
                GameState->CurrentAnimation = NewAnimation;
            }
            
            rect2 BufferRect = Rect2(0, 0, Buffer->Width, Buffer->Height);
            
#define UPDATE_CAMERA_POS 0
            
#if UPDATE_CAMERA_POS
            UpdateEntityPosViaFunction(GameState->Camera, dt);
#endif
            
            for(u32 j = 0; j < ArrayCount(GameState->WorldGrid); ++j) {
                
                u32 Y = j / WORLD_GRID_SIZE;
                u32 X = j - (Y*WORLD_GRID_SIZE);
                
                r32 MinX = X*WorldChunkInMeters;
                r32 MinY = Y*WorldChunkInMeters;
                rect2 Rect = Rect2(MinX, MinY, MinX + WorldChunkInMeters,
                                   MinY + WorldChunkInMeters);
                
                v4 Color01 = {0, 0, 0, 1};
                
                //DrawRectangle(Buffer, Rect, Color01);
                if(GameState->WorldGrid[Y*WORLD_GRID_SIZE + X]) {
                    FillRectangle(Buffer, Rect, V4(0, 0, 0, 1));
                }
            }
            
#define MOVE_VIA_ACCELERATION 0
            
            for(u32 EntityIndex = 0; EntityIndex < GameState->EntityCount; ++EntityIndex)
            {
                entity *Entity = GameState->Entities + EntityIndex;
                
                //if(Entity->FacingDirection > 0.0f) // NOTE(OLIVER): not sure what this was for??? I SHOULD COMMENT CODE MORE OFTEN!!!!!
                {
                    v2 CamPos = GameState->Camera->Pos;
                    v2 EntityRelP = Entity->Pos - CamPos;
                    
                    rect2 EntityAsRect = Rect2CenterDim(EntityRelP, Entity->Dim);
                    
                    switch(Entity->Type)
                    {
                        case((u32)Entity_Player):
                        {
                            // TODO(OLIVER): this check has to be different 
                            if(Entity->TargetPos != Entity->Pos) { 
                                
                                if(!Entity->Moving) {
                                    search_cell *SearchCellFreelist = 0;
                                    
                                    temp_memory TempMem = MarkMemory(&GameState->ScratchPad);
                                    v2i BestDirection = {};
                                    v2i StartingP = V2int(Player->TargetPos.X / WorldChunkInMeters, Player->TargetPos.Y / WorldChunkInMeters);
                                    v2i TargetP = V2int(Player->Pos.X / WorldChunkInMeters, Player->Pos.Y / WorldChunkInMeters);
                                    
                                    b32 *VisitedArray = PushArray(&GameState->ScratchPad, b32, ArrayCount(GameState->WorldGrid), true);
                                    
                                    search_cell *Sentinel = PushStruct(&GameState->ScratchPad, search_cell);
                                    
                                    Sentinel->Next = Sentinel->Prev = Sentinel;
                                    PushOnToList(&Sentinel, StartingP.X, StartingP.Y,  &GameState->ScratchPad, &SearchCellFreelist);
                                    *(VisitedArray + StartingP.Y*WORLD_GRID_SIZE + StartingP.X) = true;
                                    
                                    s32 AtX = StartingP.X;
                                    s32 AtY = StartingP.Y;
                                    b32 Found = false;
#define PUSH_TO_LIST(X, Y) Found = PushToList(GameState, X, Y, VisitedArray, &Sentinel, &SearchCellFreelist, TargetP); if(Found) break;
                                    
                                    while(Sentinel->Prev != Sentinel) {
                                        search_cell *Cell = Sentinel->Prev;
                                        AtX = Cell->X;
                                        AtY = Cell->Y;
                                        
                                        PUSH_TO_LIST(AtX + 1, AtY);
                                        PUSH_TO_LIST(AtX - 1, AtY);
                                        PUSH_TO_LIST(AtX, AtY + 1);
                                        PUSH_TO_LIST(AtX, AtY - 1);
                                        PUSH_TO_LIST(AtX + 1, AtY + 1);
                                        PUSH_TO_LIST(AtX + 1, AtY - 1);
                                        PUSH_TO_LIST(AtX - 1, AtY + 1);
                                        PUSH_TO_LIST(AtX - 1, AtY - 1);
                                        
                                        RemoveOffList(&Sentinel, Cell, &SearchCellFreelist);
                                    }
                                    
                                    ReleaseMemory(&TempMem);
                                    
                                    Entity->TempTargetPos = V2i(AtX*WorldChunkInMeters, AtY*WorldChunkInMeters);
                                    Entity->Moving = true;
                                    Entity->StartPos = Entity->Pos;
                                    Entity->MovePeriod = 0.5f;
                                    
                                    AddToOutBuffer("Temp: [%f: %f]\n", Entity->TempTargetPos.X, Entity->TempTargetPos.Y);
                                    AddToOutBuffer("Target: [%f: %f]\n", Entity->TargetPos.X, Entity->TargetPos.Y);
                                } 
                                
                                UpdatePieceOfEntityPosViaFunction(Entity, dt);
                                
                            }
                            
#if 0
                            v2 CameraWindow = V2(Buffer->Width/3, Buffer->Height/3);
                            
                            
                            // TODO(OLIVER): WORK ON CAMERA MOVEMENT!!!!!
                            if(!GameState->Camera->Moving && (Abs(EntityRelP.X) > CameraWindow.X || Abs(EntityRelP.Y) > CameraWindow.Y)) {
                                
                                GameState->Camera->Moving = true;
                                GameState->Camera->MoveT = 0;
                                
                                GameState->Camera->StartPos = GameState->Camera->Pos;
                                GameState->Camera->TargetPos = Entity->Pos - V2(Buffer->Width/2, Buffer->Height/2);
                            }
#endif
                            if(GameState->CurrentAnimation) {
                                
                                bitmap CurrentBitmap = GameState->CurrentAnimation->Frames[GameState->FrameIndex];
                                
                                GameState->FrameTime += dt;
                                if(GameState->FrameTime > GameState->FramePeriod) {
                                    GameState->FrameTime = 0;
                                    GameState->FrameIndex++;
                                    if(GameState->FrameIndex >= GameState->CurrentAnimation->FrameCount) {
                                        GameState->FrameIndex = 0;
                                    }
                                }
                                
                                
                                DrawBitmap(Buffer, &CurrentBitmap, EntityRelP.X , EntityRelP.Y, BufferRect);
                                
                                FillRectangle(Buffer, EntityAsRect, V4(1, 0, 1, 1));
                            }
                        } break;
                        case Entity_Object: {
                            
                            DrawBitmap(Buffer, &GameState->MossBlockBitmap, EntityRelP.X , EntityRelP.Y, BufferRect);
                            
                            FillRectangle(Buffer, EntityAsRect, V4(1, 0, 1, 1));
                        } break;
                    }
                }
            }
            
            DrawBitmap(Buffer, &GameState->MagicianHandBitmap, MouseP.X, MouseP.Y, BufferRect);
            
            if(GameState->RenderConsole) {
                RenderConsole(&DebugConsole); 
            }
        }
    }
    ReleaseMemory(&PerFrameMemory);
}


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
#include "calm_meta.h"

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
    
    Assert(BestResult);
    return BestResult;
}

#define GetChunkHash(X, Y) (Abs(X*19 + Y*23) % WORLD_CHUNK_HASH_SIZE)
internal world_chunk *GetWorldChunk(world_chunk **Chunks, s32 X, s32 Y, memory_arena *Arena) {
    u32 HashIndex = GetChunkHash(X, Y);
    world_chunk *Chunk = Chunks[HashIndex];
    world_chunk *Result = 0;
    
    while(Chunk) {
        
        if(Chunk->X == X && Chunk->Y == Y) {
            Result = Chunk;
            break;
        }
        
        Chunk = Chunk->Next;
    } 
    
    if(!Result && Arena) {
        Result  = PushStruct(Arena, world_chunk);
        Result->X = X;
        Result->Y = Y;
        Result->Next = Chunks[HashIndex];
        Chunks[HashIndex] = Result;
    }
    return Result;
}

inline void PushOnToList(search_cell **SentinelPtr, s32 X, s32 Y, s32 CameFromX, s32 CameFromY,  memory_arena *Arena, search_cell **FreeListPtr) {
    search_cell *Sentinel = *SentinelPtr;
    
    search_cell *NewCell = *FreeListPtr;
    
    if(NewCell) {
        *FreeListPtr = (*FreeListPtr)->Next;
    } else {
        NewCell = PushStruct(Arena, search_cell);
    }
    
    NewCell->Pos.X = X;
    NewCell->Pos.Y = Y;
    
    NewCell->CameFrom.X = CameFromX;
    NewCell->CameFrom.Y = CameFromY;
    
    NewCell->Next = Sentinel->Next;
    NewCell->Prev = Sentinel;
    
    Sentinel->Next->Prev = NewCell;
    Sentinel->Next = NewCell;
}

inline void RemoveOffList(search_cell **SentinelPtr, search_cell *CellToRemove, search_cell **FreelistPtr) {
    search_cell *Sentinel = *SentinelPtr;
    
    Assert(CellToRemove != Sentinel);
    
    CellToRemove->Next->Prev = CellToRemove->Prev;
    CellToRemove->Prev->Next = CellToRemove->Next;
    
    CellToRemove->Next = *FreelistPtr;
    *FreelistPtr = CellToRemove;
    CellToRemove->Prev = 0;
    
}

inline b32 PushToList(game_state *GameState, s32 X, s32 Y, s32 CameFromX, s32 CameFromY,  world_chunk **VisitedHash, search_cell **SentinelPtr, search_cell **FreeListPtr, v2i TargetP, 
                      memory_arena *TempArena, search_type SearchType) {
    b32 Found = false;
    
    switch(SearchType) {
        case SEARCH_VALID_SQUARES: {
            if(GetWorldChunk(GameState->Chunks, X, Y, 0) && !GetWorldChunk(VisitedHash, X, Y, 0)) {   
                
                GetWorldChunk(VisitedHash, X, Y, TempArena);
                if(X == TargetP.X && Y == TargetP.Y) { 
                    Found = true;
                } 
                PushOnToList(SentinelPtr, X, Y, CameFromX, CameFromY, TempArena, FreeListPtr); 
            }
        } break;
        case SEARCH_INVALID_SQUARES: {
            if(!GetWorldChunk(VisitedHash, X, Y, 0)) {   
                GetWorldChunk(VisitedHash, X, Y, TempArena);
                if(GetWorldChunk(GameState->Chunks, X, Y, 0)) {
                    Found = true;
                }  
                PushOnToList(SentinelPtr, X, Y, CameFromX, CameFromY, TempArena, FreeListPtr); 
            }
        } break;
        default: {
            InvalidCodePath;
        }
    }
    
    
    return Found;
}

struct search_info {
    search_cell *SearchCellList;
    world_chunk **VisitedHash;
    
    search_cell *Sentinel;
    
    b32 Found;
    
    b32 Initialized;
};

inline void InitSearchInfo(search_info *Info, v2i StartP,  game_state *GameState, memory_arena *TempArena) {
    Info->SearchCellList = 0;
    Info->VisitedHash = PushArray(TempArena, world_chunk *, ArrayCount(GameState->Chunks), true);
    
    Info->Sentinel = &GameState->SearchCellSentinel;
    
    Info->Sentinel->Next = Info->Sentinel->Prev = Info->Sentinel;
    
    Info->Found = false;
    
    Info->Initialized = true;
}

internal search_cell *CalculatePath(game_state *GameState, v2i StartP, v2i EndP,
                                    memory_arena *TempArena, search_type SearchType, search_info *Info) {
    
    s32 AtX = StartP.X;
    s32 AtY = StartP.Y;
    
#define PUSH_TO_LIST(X, Y) if(!Info->Found) {Info->Found = PushToList(GameState, X, Y, AtX, AtY, Info->VisitedHash, &Info->Sentinel, &GameState->SearchCellFreeList, EndP, TempArena, SearchType); }
    
    if(!Info->Initialized) {
        InitSearchInfo(Info, StartP, GameState, TempArena);
        PUSH_TO_LIST(AtX, AtY);
    }
    
    
    search_cell *Cell = 0;
    while(Info->Sentinel->Prev != Info->Sentinel && !Info->Found) {
        Cell = Info->Sentinel->Prev;
        AtX = Cell->Pos.X;
        AtY = Cell->Pos.Y;
        
        PUSH_TO_LIST((AtX + 1), AtY);
        PUSH_TO_LIST((AtX - 1), AtY);
        PUSH_TO_LIST(AtX, (AtY + 1));
        PUSH_TO_LIST(AtX, (AtY - 1));
        PUSH_TO_LIST((AtX + 1), (AtY + 1));
        PUSH_TO_LIST((AtX + 1), (AtY - 1));
        PUSH_TO_LIST((AtX - 1), (AtY + 1));
        PUSH_TO_LIST((AtX - 1), (AtY - 1));
        
        RemoveOffList(&Info->Sentinel, Cell, &Info->SearchCellList);
    }
    
    RemoveOffList(&Info->Sentinel, Info->Sentinel->Next, &Info->SearchCellList);
    
    return Info->SearchCellList;
}

internal void RetrievePath(game_state *GameState, v2i StartP, v2i EndP, path_nodes *Path) {
    
    
    memory_arena *TempArena = &GameState->ScratchPad;
    temp_memory TempMem = MarkMemory(TempArena);
    search_info SearchInfo = {};
    
    search_cell *SearchCellList = CalculatePath(GameState, StartP, EndP, TempArena,  SEARCH_VALID_SQUARES, &SearchInfo);
    
    Assert(SearchInfo.Found);
    
    search_cell *Cell = SearchCellList;
    
    //Compile path taken to get to the destination
    v2i LookingFor = Cell->Pos;
    while(SearchCellList) {
        b32 Found = false;
        for(search_cell **TempCellPtr = &SearchCellList; *TempCellPtr; TempCellPtr = &(*TempCellPtr)->Next) {
            search_cell *TempCell = *TempCellPtr;
            if(TempCell->Pos.X == LookingFor.X && TempCell->Pos.Y == LookingFor.Y) {
                Assert(Path->Count < ArrayCount(Path->Points));
                Path->Points[Path->Count++] = LookingFor;
                LookingFor = TempCell->CameFrom;
                *TempCellPtr = TempCell->Next;
                Found = true;
                break;
            }
        }
        
        if(!Found) {
            break;
        }
    }
    
    ReleaseMemory(&TempMem);
}

internal v2i GetClosestPosition(game_state *GameState, v2i TargetP, v2i StartP) {
    
    v2i Result = {};
    memory_arena *TempArena = &GameState->ScratchPad;
    temp_memory TempMem = MarkMemory(TempArena);
    
    search_info SearchInfo = {};
    //STAGE 2: islands;
    search_cell *SearchCellList = CalculatePath(GameState, TargetP, StartP, TempArena,  SEARCH_INVALID_SQUARES, &SearchInfo);
    
    search_cell *Cell = SearchCellList;
    Assert(SearchInfo.Found);
    
    Assert(GetWorldChunk(GameState->Chunks, Cell->Pos.X, Cell->Pos.Y, 0));
    
    Result = Cell->Pos;
    
    ReleaseMemory(&TempMem);
    return Result;
}

inline void BeginEntityPath(game_state *GameState, entity *Entity, v2i TargetP) {
    v2i StartP = V2int(Entity->Pos.X / WorldChunkInMeters, Entity->Pos.Y / WorldChunkInMeters);
    
    TargetP = GetClosestPosition(GameState, TargetP, StartP);
    
    Entity->Path.Count = 0;
    RetrievePath(GameState, TargetP, StartP, &Entity->Path);
    Entity->VectorIndexAt = 1;
}

internal void UpdateEntityPositionViaFunction(game_state *GameState, entity *Entity, r32 dt) {
    if(Entity->VectorIndexAt < Entity->Path.Count) {
        Assert(Entity->VectorIndexAt > 0);
        v2 A = WorldChunkInMeters*ToV2(Entity->Path.Points[Entity->VectorIndexAt - 1]);
        v2 B = WorldChunkInMeters*ToV2(Entity->Path.Points[Entity->VectorIndexAt]);
        
        Entity->MoveT += dt;
        
        r32 tValue = Clamp(0,Entity->MoveT / Entity->MovePeriod, 1);
        Entity->Pos = Lerp(A, tValue, B);
        
        v2 Direction = Normal(B - A);
        Entity->Velocity = 20.0f*Direction;
        
        if(Entity->MoveT / Entity->MovePeriod >= 1.0f) {
            Entity->Pos = B;
            Entity->MoveT -= Entity->MovePeriod;
            Entity->VectorIndexAt++;
            
        }
    }
}

internal b32 IsPartOfPath(s32 X, s32 Y, path_nodes *Path) {
    b32 Result = false;
    for(u32 PathIndex = 0; PathIndex < Path->Count; ++PathIndex){
        if(Path->Points[PathIndex].X == X && Path->Points[PathIndex].Y == Y) {
            Result = true;
            break;
        }
    }
    
    return Result;
}

internal void
GameUpdateAndRender(bitmap *Buffer, game_memory *Memory, r32 dt)
{
    game_state *GameState = (game_state *)Memory->GameStorage;
    r32 MetersToPixels = 60.0f;
    if(!GameState->IsInitialized)
    {
        
        InitializeMemoryArena(&GameState->MemoryArena, (u8 *)Memory->GameStorage + sizeof(game_state), Memory->GameStorageSize - sizeof(game_state));
        
        GameState->PerFrameArena = SubMemoryArena(&GameState->MemoryArena, MegaBytes(2));
        
        GameState->ScratchPad = SubMemoryArena(&GameState->MemoryArena, MegaBytes(2));
        /////////////////////////////////
        
        GameState->RenderConsole = true;
        GameState->Player = InitEntity(GameState, V2(0, 0), V2(0.5, 1), Entity_Player);
        
        GameState->Camera = InitEntity(GameState, V2(0, 0), V2(0, 0), Entity_Camera);
        
        //InitEntity(GameState, V2(2, 2), V2(1, 1), Entity_Object);
        
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
        
        for(u32 Y = 0; Y < 5; ++Y) {
            for(u32 X = 0; X < 5; ++X) {
                world_chunk *Result = GetWorldChunk(GameState->Chunks, X, Y, &GameState->MemoryArena);
                Assert(Result);
            }
        }
        
        GameState->IsInitialized = true;
    }
    
    temp_memory PerFrameMemory = MarkMemory(&GameState->PerFrameArena);
    
    render_group RenderGroup_ = {};
    RenderGroup_.MetersToPixels = 60.0f;
    RenderGroup_.ScreenDim = V2i(Buffer->Width, Buffer->Height);
    RenderGroup_.Buffer = Buffer;
    RenderGroup_.TempMem = MarkMemory(&GameState->PerFrameArena);
    RenderGroup_.Arena = SubMemoryArena(&GameState->PerFrameArena, MegaBytes(1));
    
    render_group *RenderGroup = &RenderGroup_;
    
    PushClear(RenderGroup, V4(1, 0.9f, 0.9f, 1));
    
    entity *Player = GameState->Player;
    
    switch(GameState->GameMode) {
        case MENU_MODE: {
            
            UpdateMenu(GameState, Memory, Buffer, dt);
            
        } break;
        case PLAY_MODE: {
            v2 CamPos = GameState->Camera->Pos;
            v2 MouseP = InverseTransform(RenderGroup, V2(Memory->MouseX, Memory->MouseY));
            
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
            if(WasPressed(Memory->GameButtons[Button_F1]))
            {
                DebugConsole.ViewMode = (DebugConsole.ViewMode == VIEW_MID) ? VIEW_CLOSE : VIEW_MID;
            }
            if(WasPressed(Memory->GameButtons[Button_F2]))
            {
                DebugConsole.ViewMode = (DebugConsole.ViewMode == VIEW_FULL) ? VIEW_CLOSE : VIEW_FULL;
            }
            if(WasPressed(Memory->GameButtons[Button_LeftMouse]))
            {
                if(IsDown(Memory->GameButtons[Button_Shift])) {
                    s32 CellX = (s32)(floor(MouseP.X / WorldChunkInMeters));
                    s32 CellY = (s32)(floor(MouseP.Y / WorldChunkInMeters));
                    
                    
                    if(!GetWorldChunk(GameState->Chunks, CellX, CellY, 0)) {
                        world_chunk *Chunk = GetWorldChunk(GameState->Chunks, CellX, CellY, &GameState->MemoryArena);
                        Assert(Chunk);
                        // TODO(OLIVER): Maybe make the return type a double ptr so we can assign it straight away.
                    }
                    
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
                            v2 TargetP_r32 = MouseP + GameState->Camera->Pos;
                            r32 InverseWorldChunkToMeters = 1.0f / WorldChunkInMeters;
                            
                            v2i TargetP = ToV2i(InverseWorldChunkToMeters*TargetP_r32); 
                            
                            BeginEntityPath(GameState, Player, TargetP);
                            
                            Player->OffsetTargetP = TargetP_r32 - WorldChunkInMeters*ToV2(TargetP);
                            
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
            
#if 0
            s32 StartIndex = (u32)Button_A;
            s32 EndIndex = (u32)Button_Z;
            for(s32 KeyIndex = StartIndex; KeyIndex <= EndIndex; ++KeyIndex) {
                if(IsDown(Memory->GameButtons[KeyIndex])) {
                    //char Character = GetCharacter(KeyIndex);
                    if(!IsDown(Memory->GameButtons[Button_Shift])) {
                        //Character = ToLowerCase(Character);
                    }
                    AddToInBuffer("%s", &Character);
                }
            }
#else 
            for(s32 KeyIndex = 0; KeyIndex <= Memory->SizeOfGameKeys; ++KeyIndex) {
                
                game_button *Button = Memory->GameKeys + KeyIndex;
                if(Button->IsDown) {
                    AddToInBuffer("%s", &KeyIndex);
                }
            }
#endif
            
            r32 Qualities[ANIMATE_QUALITY_COUNT] = {};
            r32 Weights[ANIMATE_QUALITY_COUNT] = {1.0f, 1.0f};
            Qualities[POSE] = Length(Player->Velocity);
            Qualities[DIRECTION] = atan2(Player->Velocity.Y, Player->Velocity.X); 
#if 0
            animation *NewAnimation = GetAnimation(GameState->LanternManAnimations, GameState->LanternAnimationCount, Qualities, Weights);
#else 
            animation *NewAnimation = &GameState->LanternManAnimations[4];
#endif
            r32 FastFactor = 1000;
            GameState->FramePeriod = 0.2f; //FastFactor / LengthSqr(Player->Velocity);
            // TODO(OLIVER): Make this into a sineous function
            GameState->FramePeriod = Clamp(0.2f, GameState->FramePeriod, 0.4f);
            
            Assert(NewAnimation);
            if(NewAnimation != GameState->CurrentAnimation) {
                GameState->FrameIndex = 0;
                GameState->CurrentAnimation = NewAnimation;
            }
            
            rect2 BufferRect = Rect2(0, 0, Buffer->Width, Buffer->Height);
            
#define UPDATE_CAMERA_POS 0
            
#if UPDATE_CAMERA_POS
            UpdateEntityPosViaFunction(GameState->Camera, dt);
#endif
            
#define DRAW_GRID 1
            
#if DRAW_GRID
            for(u32 j = 0; j < WORLD_CHUNK_HASH_SIZE; ++j) {
                
                world_chunk *Chunk = GameState->Chunks[j];
                
                while(Chunk) {
                    r32 MinX = Chunk->X*WorldChunkInMeters;
                    r32 MinY = Chunk->Y*WorldChunkInMeters;
                    rect2 Rect = Rect2(MinX, MinY, MinX + WorldChunkInMeters,
                                       MinY + WorldChunkInMeters);
                    
                    if(IsPartOfPath(Chunk->X, Chunk->Y, &Player->Path)) {
                        v4 Color01 = {1, 0, 1, 1};
                        PushRect(RenderGroup, Rect, Color01);
                    } else {
                        v4 Color01 = {0, 0, 0, 1};
                        PushRectOutline(RenderGroup, Rect, Color01);
                    }
                    
                    
                    
                    Chunk = Chunk->Next;
                }
            }
#endif
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
                            UpdateEntityPositionViaFunction(GameState, Entity, dt);
                            
#if 0
                            v2 CameraWindow = (1.0f/MetersToPixels)*V2(Buffer->Width/3, Buffer->Height/3);
                            
                            
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
                                
                                PushBitmap(RenderGroup, EntityRelP,&CurrentBitmap,  BufferRect);
                                
                                PushRect(RenderGroup, EntityAsRect, V4(1, 0, 1, 1));
                            }
                        } break;
                        case Entity_Object: {
                            
                            PushBitmap(RenderGroup, EntityRelP,&GameState->MossBlockBitmap,  BufferRect);
                            
                            PushRect(RenderGroup, EntityAsRect, V4(1, 0, 1, 1));
                        } break;
                    }
                }
            }
            
            PushBitmap(RenderGroup, MouseP,&GameState->MagicianHandBitmap,  BufferRect);
            
            
        }
    }
    
    
    RenderGroupToOutput(RenderGroup);
    
    if(GameState->RenderConsole) {
        RenderConsole(&DebugConsole); 
    }
    
    ReleaseMemory(&PerFrameMemory);
}


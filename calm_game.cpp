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
internal world_chunk *GetOrCreateWorldChunk(world_chunk **Chunks, s32 X, s32 Y, memory_arena *Arena, chunk_type Type) {
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
        Result->Type = Type;
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

inline b32 PushToList(world_chunk **WorldChunks, s32 X, s32 Y, s32 CameFromX, s32 CameFromY,  world_chunk **VisitedHash, search_cell **SentinelPtr, search_cell **FreeListPtr, v2i TargetP, 
                      memory_arena *TempArena, search_type SearchType) {
    b32 Found = false;
    
    switch(SearchType) {
        case SEARCH_VALID_SQUARES: {
            if(GetOrCreateWorldChunk(WorldChunks, X, Y, 0, chunk_null) && !GetOrCreateWorldChunk(VisitedHash, X, Y, 0, chunk_null)) {   
                
                GetOrCreateWorldChunk(VisitedHash, X, Y, TempArena, chunk_null);
                if(X == TargetP.X && Y == TargetP.Y) { 
                    Found = true;
                } 
                PushOnToList(SentinelPtr, X, Y, CameFromX, CameFromY, TempArena, FreeListPtr); 
            }
        } break;
        case SEARCH_INVALID_SQUARES: { 
            if(!GetOrCreateWorldChunk(VisitedHash, X, Y, 0, chunk_null)) {   
                GetOrCreateWorldChunk(VisitedHash, X, Y, TempArena, chunk_null);
                if(GetOrCreateWorldChunk(WorldChunks, X, Y, 0, chunk_null)) {
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

inline void InitSearchInfo(search_info *Info,  game_state *GameState, memory_arena *TempArena) {
    Info->SearchCellList = 0;
    Info->VisitedHash = PushArray(TempArena, world_chunk *, ArrayCount(GameState->Chunks), true);
    
    Info->Sentinel = &GameState->SearchCellSentinel;
    
    Info->Sentinel->Next = Info->Sentinel->Prev = Info->Sentinel;
    
    Info->Found = false;
    
    Info->Initialized = true;
}

internal search_cell *CalculatePath(game_state *GameState, world_chunk **WorldChunks, v2i StartP, v2i EndP,
                                    memory_arena *TempArena, search_type SearchType, search_info *Info) {
    
    s32 AtX = StartP.X;
    s32 AtY = StartP.Y;
    
#define PUSH_TO_LIST(X, Y) if(!Info->Found) {Info->Found = PushToList(WorldChunks, X, Y, AtX, AtY, Info->VisitedHash, &Info->Sentinel, &GameState->SearchCellFreeList, EndP, TempArena, SearchType); }
    
    if(!Info->Initialized) {
        InitSearchInfo(Info, GameState, TempArena);
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
#if MOVE_DIAGONAL
        PUSH_TO_LIST((AtX + 1), (AtY + 1));
        PUSH_TO_LIST((AtX + 1), (AtY - 1));
        PUSH_TO_LIST((AtX - 1), (AtY + 1));
        PUSH_TO_LIST((AtX - 1), (AtY - 1));
#endif
        
        RemoveOffList(&Info->Sentinel, Cell, &Info->SearchCellList);
    }
    
    if(Info->Found) {
        RemoveOffList(&Info->Sentinel, Info->Sentinel->Next, &Info->SearchCellList);
    }
    
    
    return Info->SearchCellList;
}


internal void RetrievePath(game_state *GameState, v2i StartP, v2i EndP, path_nodes *Path) {
    
    
    memory_arena *TempArena = &GameState->ScratchPad;
    temp_memory TempMem = MarkMemory(TempArena);
    search_info SearchInfo = {};
    
    search_cell *SearchCellList = CalculatePath(GameState, GameState->Chunks, StartP, EndP, TempArena,  SEARCH_VALID_SQUARES, &SearchInfo);
    
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
                Path->Points[Path->Count++] = ToV2(LookingFor);
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

/*NOTE(Ollie): This works by first flood filling the island
the player is situated on using the visited hash and setting the targetPos to somwhere impossible (a very big number). This gives us all 
valid positions the player could move to. We then find the closest 
valid cell to where the player clicked, only searching the hash of
valid cells returned to us by the last funciton. 
*/
internal v2i GetClosestPosition(game_state *GameState, v2i TargetP, v2i StartP) {
    
    v2i Result = {};
    memory_arena *TempArena = &GameState->ScratchPad;
    temp_memory TempMem = MarkMemory(TempArena);
    
    search_info FirstStageSearchInfo = {};
    CalculatePath(GameState, GameState->Chunks, StartP, V2int(MAX_S32, MAX_S32), TempArena,  SEARCH_VALID_SQUARES, &FirstStageSearchInfo);
    
    Assert(!FirstStageSearchInfo.Found);
    
    search_info SecondStageSearchInfo = {};
    
    search_cell *SearchCellList = CalculatePath(GameState, FirstStageSearchInfo.VisitedHash, TargetP, StartP, TempArena,  SEARCH_INVALID_SQUARES, &SecondStageSearchInfo);
    search_cell *Cell = SearchCellList;
    Assert(SecondStageSearchInfo.Found);
    
    Assert(GetOrCreateWorldChunk(GameState->Chunks, Cell->Pos.X, Cell->Pos.Y, 0, chunk_null));
    
    Result = Cell->Pos;
    
    ReleaseMemory(&TempMem);
    return Result;
}

inline void BeginEntityPath(game_state *GameState, entity *Entity, v2i TargetP) { //TargetP is rounded; no offset
    v2i StartP = V2int(Entity->Pos.X / WorldChunkInMeters, Entity->Pos.Y / WorldChunkInMeters);
    
    TargetP = GetClosestPosition(GameState, TargetP, StartP);
    
    Entity->Path.Count = 0;
    RetrievePath(GameState, TargetP, StartP, &Entity->Path);
    v2 * EndPoint = Entity->Path.Points + (Entity->Path.Count - 1);
    v2 *StartPoint = Entity->Path.Points;
    *StartPoint = *StartPoint + Entity->BeginOffsetTargetP;
#if END_WITH_OFFSET
    *EndPoint = *EndPoint + Entity->EndOffsetTargetP;
#endif
    Entity->VectorIndexAt = 1;
}

internal void UpdateEntityPositionViaFunction(game_state *GameState, entity *Entity, r32 dt) {
    if(Entity->VectorIndexAt < Entity->Path.Count) {
        Assert(Entity->VectorIndexAt > 0);
        v2 A = WorldChunkInMeters*Entity->Path.Points[Entity->VectorIndexAt - 1];
        v2 B = WorldChunkInMeters*Entity->Path.Points[Entity->VectorIndexAt];
        
        Entity->MoveT += dt;
        
        r32 Factor = (Length(B - A)/WorldChunkInMeters);
        r32 MovePeriod = Entity->MovePeriod*Factor;
        
        r32 tValue = Clamp(0,Entity->MoveT / MovePeriod, 1);
        if(Entity->VectorIndexAt == 1) {
            //Entity->Pos = ExponentialUpLerp0To1(A, tValue, B);
        }
        else if(Entity->VectorIndexAt == (Entity->Path.Count - 1)) {
            //Entity->Pos = ExponentialDownLerp0To1(A, tValue, B);
        } else {
            
        }
        Entity->Pos = Lerp(A, tValue, B);
        
        v2 Direction = Normal(B - A);
        Entity->Velocity = 20.0f*Direction;
        
        if(Entity->MoveT / MovePeriod >= 1.0f) {
            Entity->Pos = B;
            Entity->MoveT -= MovePeriod;
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
        GameState->Player = InitEntity(GameState, V2(0, 0), V2(WorldChunkInMeters, WorldChunkInMeters), Entity_Player);
        
        GameState->Camera = InitEntity(GameState, V2(0, 0), V2(0, 0), Entity_Camera);
        
        InitEntity(GameState, V2(2, 2), V2(1, 1), Entity_Guru, true);
        
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
        
        font_quality_value Qualities[FontQuality_Count] = {};
        Qualities[FontQuality_Debug] = InitFontQualityValue(1.0f);
        Qualities[FontQuality_Size] = InitFontQualityValue(1.0f);
        
        //GameState->Font = FindFont(GameState->Fonts, Qualities);
        GameState->DebugFont = FindFontByName(GameState->Fonts, "Liberation Mono", Qualities);
        GameState->GameFont = FindFontByName(GameState->Fonts, "Karmina Regular", Qualities);
        
        InitConsole(&DebugConsole, GameState->DebugFont, 0, 2.0f, &GameState->MemoryArena);
        
        GameState->MenuPeriod = 2.0f;
        
        GameState->FramePeriod = 0.2f;
        GameState->FrameIndex = 0;
        GameState->FrameTime = 0;
        GameState->CurrentAnimation = &GameState->LanternManAnimations[0];
        
        s32 Range = 10;
        fori_count(700) {
            s32 Y = RandomBetween(&GameState->GeneralEntropy, -Range, Range);
            s32 X = RandomBetween(&GameState->GeneralEntropy, -Range, Range);
            
            u32 ChunkType = RandomChoice(&GameState->GeneralEntropy, ((u32)chunk_type_count - 1)) + 1;
            world_chunk *Result = GetOrCreateWorldChunk(GameState->Chunks, X, Y, &GameState->MemoryArena, (chunk_type)ChunkType);
            Assert(Result);
            
        }
        
        GameState->IsInitialized = true;
    }
    
    temp_memory PerFrameMemory = MarkMemory(&GameState->PerFrameArena);
    
    render_group OrthoRenderGroup;
    InitRenderGroup(&OrthoRenderGroup, Buffer, &GameState->PerFrameArena, MegaBytes(1), V2(1, 1), V2(0, 0), V2(1, 0));
    DebugConsole.RenderGroup = &OrthoRenderGroup;
    
    render_group RenderGroup_;
    InitRenderGroup(&RenderGroup_, Buffer, &GameState->PerFrameArena, MegaBytes(1), V2(60, 60), 0.5f*V2(Buffer->Width, Buffer->Height), V2(1, 0));
    
    render_group *RenderGroup = &RenderGroup_;
    
    PushClear(RenderGroup, V4(1, 0.9f, 0.9f, 1));
    
    entity *Player = GameState->Player;
    
    switch(GameState->GameMode) {
        case MENU_MODE: {
            UpdateMenu(GameState, Memory, &OrthoRenderGroup, dt);
            GameState->RenderConsole = false;
            
        } break;
        case PLAY_MODE: {
            GameState->RenderConsole = true;
            v2 CamPos = GameState->Camera->Pos;
            v2 MouseP = InverseTransform(&RenderGroup->Transform, V2(Memory->MouseX, Memory->MouseY));
            
            if(WasPressed(Memory->GameButtons[Button_Escape]))
            {
                GameState->GameMode = MENU_MODE;
                break;
            }
            if(WasPressed(Memory->GameButtons[Button_F1]))
            {
                if(DebugConsole.ViewMode == VIEW_MID) {
                    DebugConsole.ViewMode = VIEW_CLOSE;
                    DebugConsole.InputIsActive = false;
                } else {
                    DebugConsole.ViewMode = VIEW_MID;
                    DebugConsole.InputIsActive = true;
                    DebugConsole.InputClickTimer.Value = 0;
                }
                
            }
            if(WasPressed(Memory->GameButtons[Button_F2]))
            {
                if(DebugConsole.ViewMode == VIEW_FULL) {
                    DebugConsole.ViewMode = VIEW_CLOSE;
                    DebugConsole.InputIsActive = false;
                } else {
                    DebugConsole.ViewMode = VIEW_FULL;
                    DebugConsole.InputIsActive = true;
                    DebugConsole.InputClickTimer.Value = 0;
                }
            }
            
            if(WasPressed(Memory->GameButtons[Button_LeftMouse]))
            {
                if(InBounds(DebugConsole.InputConsoleRect, Memory->MouseX, Memory->MouseY)) {
                    DebugConsole.InputIsHot = true;
                }
            }
            if(WasReleased(Memory->GameButtons[Button_LeftMouse])) {
                
                if(InBounds(DebugConsole.InputConsoleRect, Memory->MouseX, Memory->MouseY) && DebugConsole.InputIsHot) {
                    DebugConsole.InputIsHot = false;
                    DebugConsole.InputIsActive = true;
                    DebugConsole.InputClickTimer.Value = 0;
                } else if(!DebugConsole.InputIsHot){
                    DebugConsole.InputIsActive = false;
                }
            }
            
            if(DebugConsole.InputIsActive) {
                if(WasPressed(Memory->GameButtons[Button_Arrow_Left])) {
                    DebugConsole.Input.WriteIndexAt = Max(0, (s32)DebugConsole.Input.WriteIndexAt - 1);
                    DebugConsole.InputTimer.Value = 0;
                }
                if(WasPressed(Memory->GameButtons[Button_Arrow_Right])) {
                    DebugConsole.Input.WriteIndexAt = Min(DebugConsole.Input.IndexAt, DebugConsole.Input.WriteIndexAt + 1);
                    DebugConsole.InputTimer.Value = 0;
                }
                for(s32 KeyIndex = 0; KeyIndex <= Memory->SizeOfGameKeys; ++KeyIndex) {
                    
                    game_button *Button = Memory->GameKeys + KeyIndex;
                    
                    if(Button->IsDown) {
                        DebugConsole.InputTimer.Value = 0;
                        switch(KeyIndex) {
                            case 8: { //Backspace
                                Splice_("", &DebugConsole.Input, -1);
                            } break;
                            default: {
                                AddToInBuffer("%s", &KeyIndex);
                            }
                        }
                    }
                }
                if(WasPressed(Memory->GameButtons[Button_Enter])) {
                    IssueCommand(&DebugConsole.Input);
                    ClearBuffer(&DebugConsole.Input);
                }
                
            } else {
                
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
                
                if(WasPressed(Memory->GameButtons[Button_LeftMouse])) {
                    if(IsDown(Memory->GameButtons[Button_Shift])) {
                        s32 CellX = (s32)(floor(MouseP.X / WorldChunkInMeters));
                        s32 CellY = (s32)(floor(MouseP.Y / WorldChunkInMeters));
                        
                        
                        if(!GetOrCreateWorldChunk(GameState->Chunks, CellX, CellY, 0, chunk_null)) {
                            world_chunk *Chunk = GetOrCreateWorldChunk(GameState->Chunks, CellX, CellY, &GameState->MemoryArena, chunk_grass);
                            Assert(Chunk);
                            // TODO(OLIVER): Maybe make the return type a double ptr so we can assign it straight away.
                        }
                        
                    } else {
                        entity *NextHotEntity = 0;
                        for(u32 i = 0; i < GameState->EntityCount; ++i) {
                            entity *Entity = GameState->Entities + i;
                            if(Entity->IsInteractable) {
                                
                                v2 EntityRelP = Entity->Pos - CamPos;
                                
                                rect2 EntityAsRect = Rect2CenterDim(EntityRelP, Entity->Dim);
                                if(InBounds(EntityAsRect, MouseP)) {
                                    NextHotEntity = Entity;
                                }
                            }
                        }
                        
                        if(!GameState->InteractingWith) {
                            if(!NextHotEntity) {
                                //NOTE: This is where we move the player.
                                v2 TargetP_r32 = MouseP + GameState->Camera->Pos;
                                r32 MetersToWorldChunks = 1.0f / WorldChunkInMeters;
                                
                                v2i TargetP = ToV2i_floor(MetersToWorldChunks*TargetP_r32); 
                                
                                Player->BeginOffsetTargetP = Player->Pos- WorldChunkInMeters*ToV2i(MetersToWorldChunks*Player->Pos); ;
                                
                                Player->EndOffsetTargetP = TargetP_r32 - WorldChunkInMeters*ToV2(TargetP);
                                
                                BeginEntityPath(GameState, Player, TargetP);
                                Player->MoveT = 0;
                            } else {
                                //NOTE: This is interactable objects;
                                GameState->HotEntity = NextHotEntity;
                                GameState->InteractingWith = GameState->HotEntity;
                            }
                        }
                    }
                }
                if(WasReleased(Memory->GameButtons[Button_LeftMouse])) {
                    
                    //MOUSE BUTTON RELEASED ACTIONS
                    if(GameState->InteractingWith) {
                        switch(GameState->InteractingWith->Type) {
                            case Entity_Guru: {
                                GameState->InteractingWith->TriggerAction = true;
                            }
                        }
                        
                        GameState->InteractingWith = 0;
                    }
                }
                
                if(GameState->InteractingWith) {
                    //MOUSE BUTTON DOWN ACTIONS
                    switch(GameState->InteractingWith->Type) {
                        case Entity_Moveable_Object: {
                            GameState->InteractingWith->Pos = MouseP + CamPos;
                        } break;
                        default: {
                            AddToOutBuffer("There is no release interaction for the object type %d\n", GameState->InteractingWith->Type); //TODO(oliver): This would be a good place for a meta program: All enum structs could have a method GetEnumAlias(entity_type, TypeValue);
                        }
                    }
                    
                    
                }
            }
            
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
            
#define UPDATE_CAMERA_POS 1
            
#if UPDATE_CAMERA_POS
            
            entity *Camera = GameState->Camera;
            r32 DragFactor = 0.4f;
            v2 Acceleration = Camera->AccelFromLastFrame;
            Camera->Pos = dt*dt*Acceleration + dt*Camera->Velocity + Camera->Pos;
            Camera->Velocity = Camera->Velocity + dt*Acceleration - DragFactor*Camera->Velocity;
            
#endif
            
#define DRAW_GRID 1
            
#if DRAW_GRID
            for(u32 j = 0; j < WORLD_CHUNK_HASH_SIZE; ++j) {
                
                world_chunk *Chunk = GameState->Chunks[j];
                
                while(Chunk) {
                    r32 MinX = Chunk->X*WorldChunkInMeters - GameState->Camera->Pos.X;
                    r32 MinY = Chunk->Y*WorldChunkInMeters - GameState->Camera->Pos.Y;
                    rect2 Rect = Rect2(MinX, MinY, MinX + WorldChunkInMeters,
                                       MinY + WorldChunkInMeters);
                    
                    v4 Color01 = {0, 0, 0, 1};
                    switch (Chunk->Type) {
                        case chunk_grass: {
                            Color01 = {0, 1, 0, 1};
                        } break;
                        case chunk_fire: {
                            Color01 = {1, 0, 0, 1};
                        } break;
                        case chunk_water: {
                            Color01 = {0, 0, 1, 1};
                        } break;
                        default: {
                            InvalidCodePath;
                        }
                        
                    }
                    
                    if(IsPartOfPath(Chunk->X, Chunk->Y, &Player->Path)) {
                        Color01 = {1, 0, 1, 1};
                        PushRect(RenderGroup, Rect, 1, Color01);
                    } else {
                        PushRect(RenderGroup, Rect, 1, Color01);
                    }
                    
                    
                    
                    Chunk = Chunk->Next;
                }
            }
#endif
            
            
            r32 PercentOfScreen = 0.1f;
            v2 CameraWindow = (1.0f/MetersToPixels)*PercentOfScreen*V2(Buffer->Width, Buffer->Height);
            //NOTE(): Draw Camera bounds that the player stays within. 
            PushRectOutline(RenderGroup, Rect2(-CameraWindow.X, -CameraWindow.Y, CameraWindow.X, CameraWindow.Y), 1, V4(1, 0, 1, 1));
            ////
            for(u32 EntityIndex = 0; EntityIndex < GameState->EntityCount; ++EntityIndex)
            {
                entity *Entity = GameState->Entities + EntityIndex;
                
                //if(Entity->FacingDirection > 0.0f) // NOTE(OLIVER): not sure what this was for??? I SHOULD COMMENT CODE MORE OFTEN!!!!!
                {
                    v2 CamPos = GameState->Camera->Pos;
                    v2 EntityRelP = Entity->Pos - CamPos;
                    
                    rect2 EntityAsRect = Rect2MinDim(EntityRelP, Entity->Dim);
                    
                    switch(Entity->Type)
                    {
                        case((u32)Entity_Player):
                        {
                            UpdateEntityPositionViaFunction(GameState, Entity, dt);
                            
                            if((Abs(EntityRelP.X) > CameraWindow.X || Abs(EntityRelP.Y) > CameraWindow.Y)) {
                                Camera->AccelFromLastFrame = 10.0f*EntityRelP;
                            } else {
                                Camera->AccelFromLastFrame = V2(0, 0);
                            }
                            
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
                                
                                //PushBitmap(RenderGroup, V3(EntityRelP, 1), &CurrentBitmap,  BufferRect);
                                
                                PushRect(RenderGroup, EntityAsRect, 1, V4(0, 1, 1, 1));
                            }
                        } break;
                        case Entity_Moveable_Object: {
                            //PushRect(RenderGroup, EntityAsRect, 1, V4(1, 0, 1, 1));
                            PushBitmap(RenderGroup, V3(EntityRelP, 1), &GameState->MossBlockBitmap,  BufferRect);
                        } break;
                        case((u32)Entity_Guru): {
                            PushRect(RenderGroup, EntityAsRect, 1, V4(1, 0, 1, 1));
                            if(Entity->TriggerAction) {
                                //AddToOutBuffer("TriggerAction");
                                draw_text_options TextOptions = InitDrawOptions();
                                TextOptions.AdvanceYAtStart = true;
                                char *DialogString = "Hello there weary traveller, how have you been?";
                                rect2 Bounds = Rect2MinMaxWithCheck(V2(EntityRelP.X, EntityRelP.Y), V2(EntityRelP.X + 4, EntityRelP.Y - 4));
                                PushRectOutline(RenderGroup, Bounds, 1, V4(0, 1, 1, 1));
                                
                                TextToOutput(RenderGroup, GameState->GameFont, DialogString, EntityRelP.X, EntityRelP.Y, Bounds, V4(0, 0, 0, 1), &TextOptions);
                                
                                
                            }
                            
                            //PushBitmap(RenderGroup, V3(EntityRelP, 1), &GameState->MossBlockBitmap,  BufferRect);
                        } break;
                    }
                }
            }
            
            PushBitmap(RenderGroup, V3(MouseP, 1), &GameState->MagicianHandBitmap,  BufferRect);
        }
    }
    
    
    if(GameState->RenderConsole) {
        RenderConsole(&DebugConsole, dt); 
    }
    
    RenderGroupToOutput(RenderGroup);
    RenderGroupToOutput(&OrthoRenderGroup);
    
    ReleaseMemory(&PerFrameMemory);
}


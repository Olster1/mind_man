/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#include "calm_game.h"
#include "calm_audio.cpp"
#include "calm_render.cpp"
#include "calm_menu.cpp"
#include "calm_console.cpp"
#include "calm_particles.cpp"
#include "calm_ui.h"
#include "calm_animation.cpp"
#include "calm_world_chunk.cpp"
#include "calm_entity.cpp"
#include "calm_meta.h"
#include "meta_enum_arrays.h"
#include "meta_introspect_struct_arrays.h"


inline void SetEditorMode(ui_state *UIState, editor_mode Mode) {
    UIState->EditorMode = {(u32)Mode, editor_mode_Values, editor_mode_Names[(u32)Mode]};
}

inline void SetEntityTypeMode(ui_state *UIState, entity_type Type) {
    UIState->InitInfo = {(u32)Type, entity_type_Values, entity_type_Names[(u32)Type]};
}


internal inline tile_pos_type GetTilePosType(world_chunk **Chunks, s32 X, s32 Y) {
    tile_pos_type Result = NULL_TILE;
    
    /* We Don't yet handle the diagonal case so will get an assert;*/
    
#define WorldChunkTest(XAdd, YAdd) Spots[YAdd + 1][XAdd + 1] = (GetOrCreateWorldChunk(Chunks, X + XAdd, Y + YAdd, 0, ChunkNull)) ? 1 : 0;
    
    tile_type_layout Layouts[10];
    u32 IndexAt = 0;
    Layouts[IndexAt++] = {TOP_LEFT_TILE, {
            {0, 0, 0}, 
            {0, 1, 1}, 
            {0, 1, 0}
        }
    };
    Layouts[IndexAt++] = {TOP_CENTER_TILE, {
            {0, 0, 0}, 
            {1, 1, 1}, 
            {0, 1, 0}
        }
    };
    Layouts[IndexAt++] = {TOP_RIGHT_TILE, {
            {0, 0, 0}, 
            {1, 1, 0}, 
            {0, 1, 0}
        }
    };
    Layouts[IndexAt++] = {CENTER_LEFT_TILE, {
            {0, 1, 0}, 
            {0, 1, 1}, 
            {0, 1, 0}
        }
    };
    Layouts[IndexAt++] = {CENTER_TILE, {
            {0, 1, 0}, 
            {1, 1, 1}, 
            {0, 1, 0}
        }
    };
    Layouts[IndexAt++] = {CENTER_TILE, {
            {0, 0, 0}, 
            {0, 1, 0}, 
            {0, 0, 0}
        }
    };
    Layouts[IndexAt++] = {CENTER_RIGHT_TILE, {
            {0, 1, 0}, 
            {1, 1, 0}, 
            {0, 1, 0}
        }
    };
    Layouts[IndexAt++] = {BOTTOM_LEFT_TILE, {
            {0, 1, 0}, 
            {0, 1, 1}, 
            {0, 0, 0}
        }
    };
    Layouts[IndexAt++] = {BOTTOM_CENTER_TILE, {
            {0, 1, 0}, 
            {1, 1, 1}, 
            {0, 0, 0}
        }
    };
    Layouts[IndexAt++] = {BOTTOM_RIGHT_TILE, {
            {0, 1, 0}, 
            {1, 1, 0}, 
            {0, 0, 0}
        }
    };
    
    s32 Spots[3][3] = {};
    
    WorldChunkTest(-1, 1);
    WorldChunkTest(0, 1);
    WorldChunkTest(1, 1);
    
    WorldChunkTest(-1, 0);
    WorldChunkTest(0, 0);
    WorldChunkTest(1, 0);
    
    WorldChunkTest(-1, -1);
    WorldChunkTest(0, -1);
    WorldChunkTest(1, -1);
    
    fori(Layouts) {
        tile_type_layout *Layout = Layouts + Index;
#define IsEqual_PosTile(Y, X) Layout->E[Y][X] == Spots[Y][X]
        if(IsEqual_PosTile(0, 1) && 
           IsEqual_PosTile(1, 0) && 
           IsEqual_PosTile(0, 1) && 
           IsEqual_PosTile(1, 1)) {
            Result = Layout->Type;
            break;
        }
        
    }
    
    //Assert(Result != NULL_TILE);
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

inline b32 IsValidType(world_chunk *Chunk, chunk_type *Types, u32 TypeCount) {
    b32 Result = false;
    forN(TypeCount) {
        chunk_type Type = Types[TypeCountIndex];
        //Assert(Type != ChunkBlock);
        if(Type == Chunk->Type) {
            Result = true;
            break;
        }
    }
    return Result;
}

inline b32 PushToList(world_chunk **WorldChunks, s32 X, s32 Y, s32 CameFromX, s32 CameFromY,  world_chunk **VisitedHash, search_cell **SentinelPtr, search_cell **FreeListPtr, v2i TargetP, 
                      memory_arena *TempArena, search_type SearchType, chunk_type *ChunkTypes, u32 ChunkTypeCount) {
    b32 Found = false;
    
    switch(SearchType) {
        case SEARCH_VALID_SQUARES: {world_chunk *Chunk = GetOrCreateWorldChunk(WorldChunks, X, Y, 0, ChunkTypes[0]);
            if(Chunk && IsValidType(Chunk, ChunkTypes, ChunkTypeCount) && !GetOrCreateWorldChunk(VisitedHash, X, Y, 0, ChunkTypes[0])) {   
                
                world_chunk *VisitedChunk = GetOrCreateWorldChunk(VisitedHash, X, Y, TempArena, ChunkTypes[0]);
                VisitedChunk->MainType = Chunk->Type;
                
                if(X == TargetP.X && Y == TargetP.Y) { 
                    Found = true;
                } 
                PushOnToList(SentinelPtr, X, Y, CameFromX, CameFromY, TempArena, FreeListPtr); 
            }
        } break;
        case SEARCH_FOR_TYPE: { 
            if(!GetOrCreateWorldChunk(VisitedHash, X, Y, 0, ChunkTypes[0])) {   
                GetOrCreateWorldChunk(VisitedHash, X, Y, TempArena, ChunkTypes[0]); 
                world_chunk *Chunk = GetOrCreateWorldChunk(WorldChunks, X, Y, 0, ChunkTypes[0]);
                if(Chunk && Chunk->MainType == ChunkTypes[0]) { 
                    Found = true;
                }  
                PushOnToList(SentinelPtr, X, Y, CameFromX, CameFromY, TempArena, FreeListPtr); 
            }
        } break;
        case SEARCH_INVALID_SQUARES: { 
            if(!GetOrCreateWorldChunk(VisitedHash, X, Y, 0, ChunkTypes[0])) {   
                GetOrCreateWorldChunk(VisitedHash, X, Y, TempArena, ChunkTypes[0]); 
                world_chunk *Chunk = GetOrCreateWorldChunk(WorldChunks, X, Y, 0, ChunkTypes[0]);
                if(Chunk && IsValidType(Chunk, ChunkTypes, ChunkTypeCount)) { 
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
    Info->VisitedHash = PushArray(TempArena, world_chunk *, ArrayCount(GameState->Chunks));
    ClearArray(Info->VisitedHash, world_chunk *, ArrayCount(GameState->Chunks));
    
    Info->Sentinel = &GameState->SearchCellSentinel;
    
    Info->Sentinel->Next = Info->Sentinel->Prev = Info->Sentinel;
    
    Info->Found = false;
    
    Info->Initialized = true;
}

internal search_cell *CalculatePath(game_state *GameState, world_chunk **WorldChunks, v2i StartP, v2i EndP,
                                    memory_arena *TempArena, search_type SearchType, search_info *Info, chunk_type *ChunkTypes, u32 ChunkTypeCount) {
    
    /* 
    We are doing this because for the other search types we flip the direction around for the AI search. Maybe look into this to get a better idea so we don't have to have this.
    */
    if(SearchType == SEARCH_FOR_TYPE) {
        v2i TempP = StartP;
        StartP = EndP;
        EndP = TempP;
    }
    
    s32 AtX = StartP.X;
    s32 AtY = StartP.Y;
    
#define PUSH_TO_LIST(X, Y) if(!Info->Found) {Info->Found = PushToList(WorldChunks, X, Y, AtX, AtY, Info->VisitedHash, &Info->Sentinel, &GameState->SearchCellFreeList, EndP, TempArena, SearchType, ChunkTypes,ChunkTypeCount); }
    
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

//This returns a boolean if the location we were looking for was found. 
internal b32 RetrievePath(game_state *GameState, v2i StartP, v2i EndP, path_nodes *Path, chunk_type *ChunkTypes, u32 ChunkTypeCount) {
    b32 Result = false;
    
    memory_arena *TempArena = &GameState->ScratchPad;
    temp_memory TempMem = MarkMemory(TempArena);
    search_info SearchInfo = {};
    
    search_cell *SearchCellList = CalculatePath(GameState, GameState->Chunks, StartP, EndP, TempArena,  SEARCH_VALID_SQUARES, &SearchInfo, ChunkTypes, ChunkTypeCount);
    
    // NOTE(Oliver): This will fire if the player is standing on a light world piece but is not connected to the philosopher's area. 
    // TODO(Oliver): Do we want to change this to an 'if' to account for when the player is out of the zone. Like abort the search and go back to random walking?
    if(SearchInfo.Found) {
        Result = true;
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
        
        
    }
    
    ReleaseMemory(&TempMem);
    
    return Result;
}

//NOTE(Ollie): Pulled this out from the below code to use elsewhere. 
internal void ComputeValidArea(game_state *GameState, search_info *SearchInfo, v2i StartP, chunk_type *ChunkTypes, u32 ChunkTypeCount, memory_arena *TempArena) {
    
    CalculatePath(GameState, GameState->Chunks, StartP, V2int(MAX_S32, MAX_S32), TempArena,  SEARCH_VALID_SQUARES, SearchInfo, ChunkTypes, ChunkTypeCount);
    
    Assert(!SearchInfo->Found);
}



/*NOTE(Ollie): This works by first flood filling the island
the player is situated on using the visited hash and setting the targetPos to somwhere impossible (a very big number). This gives us all 
valid positions the player could move to. We then find the closest 
valid cell to where the player clicked, only searching the hash of
valid cells returned to us by the last funciton. 
*/
internal v2i SearchForClosestValidPosition_(game_state *GameState, v2i TargetP, v2i StartP, chunk_type *ChunkTypes, u32 ChunkTypeCount, search_type SearchType) {
    
    v2i Result = {};
    memory_arena *TempArena = &GameState->ScratchPad;
    temp_memory TempMem = MarkMemory(TempArena);
    //Make valid square search area by choosing a really far away position
    
    search_info FirstStageSearchInfo = {};
    ComputeValidArea(GameState, &FirstStageSearchInfo, StartP, ChunkTypes, ChunkTypeCount, TempArena);
    //Find the target within the valid search area. 
    search_info SecondStageSearchInfo = {};
    
    search_cell *SearchCellList = CalculatePath(GameState, FirstStageSearchInfo.VisitedHash, TargetP, StartP, TempArena, SearchType, &SecondStageSearchInfo, ChunkTypes, ChunkTypeCount);
    search_cell *Cell = SearchCellList;
    Assert(SecondStageSearchInfo.Found); //Not sure about this Assert: I think it can be found or not found 
    
    Assert(GetOrCreateWorldChunk(GameState->Chunks, Cell->Pos.X, Cell->Pos.Y, 0, ChunkTypes[0]));
    
    Result = Cell->Pos;
    
    ReleaseMemory(&TempMem);
    return Result;
}

internal v2i SearchForClosestValidPosition(game_state *GameState, v2i TargetP, v2i StartP, chunk_type *ChunkTypes, u32 ChunkTypeCount) {
    v2i Result = SearchForClosestValidPosition_(GameState, TargetP, StartP, ChunkTypes, ChunkTypeCount, SEARCH_INVALID_SQUARES);
    return Result;
}

internal v2i SearchForClosestValidChunkType(game_state *GameState, v2i StartP, chunk_type *ChunkTypes, u32 ChunkTypeCount) {
    v2i Result = SearchForClosestValidPosition_(GameState, V2int(MAX_S32, MAX_S32), StartP, ChunkTypes, ChunkTypeCount, SEARCH_FOR_TYPE);
    return Result;
}

inline void SetChunkType(game_state *GameState, v2 Pos, chunk_type ChunkType) {
    v2i GridLocation = GetGridLocation(Pos);
    world_chunk *Chunk = GetOrCreateWorldChunk(GameState->Chunks, (s32)GridLocation.X, (s32)GridLocation.Y, 0, ChunkNull);
    
    Assert(ChunkType != ChunkNull);
    if(Chunk){
        if(ChunkType == ChunkMain) {
            Chunk->Type = Chunk->MainType;
        } else {
            Chunk->Type = ChunkType;
        }
    }
}

inline b32 IsValidGridPosition(game_state *GameState, v2 WorldPos) {
    v2i GridPos = GetClosestGridLocation(WorldPos);
    world_chunk *Chunk = GetOrCreateWorldChunk(GameState->Chunks, GridPos.X, GridPos.Y, 0, ChunkNull);
    b32 Result = false;
    if(Chunk) {
        Result = true;
    }
    return Result;
}

inline b32 HasMovesLeft(entity *Entity) {
    b32 Result = Entity->VectorIndexAt < Entity->Path.Count;
    if(Entity->Type == Entity_Philosopher ) {
        BreakPoint;
    }
    return Result;
}

internal inline r32 GetFractionOfMove(entity *Entity) {
    r32 tValue = Clamp(0, Entity->MoveT / Entity->MovePeriod, 1);
    return tValue;
}

internal inline b32 CanMove(entity *Entity, r32 PercentOfMoveFinished = 1.0f) {
    b32 Result = false;
    if(!HasMovesLeft(Entity) || GetFractionOfMove(Entity) > PercentOfMoveFinished) {
        Result = true;
    }
    
    return Result;
}

internal inline b32 IsValidChunkType(game_state *GameState, entity *Entity, v2i Pos, b32 Break = false) {
    b32 Result = false;
    world_chunk *ChunkHeadingFor = GetOrCreateWorldChunk(GameState->Chunks, Pos.X, Pos.Y, 0, ChunkNull);
    if(ChunkHeadingFor) {
        Result = ContainsChunkType(Entity, ChunkHeadingFor->Type);
    }
    
    if(Break && !Result) {
        BreakPoint;
    }
    return Result;
}

internal void UpdateEntityPositionViaFunction(game_state *GameState, entity *Entity, r32 dt) {
    if(HasMovesLeft(Entity)) {
        Entity->WentToNewMove = false;
        Assert(Entity->VectorIndexAt > 0);
        v2i GridA = ToV2i(Entity->Path.Points[Entity->VectorIndexAt - 1]);
        v2 A = WorldChunkInMeters*V2i(GridA);
        
        v2i GridB = ToV2i(Entity->Path.Points[Entity->VectorIndexAt]);
        v2 B = WorldChunkInMeters*V2i(GridB);
        
        v2i StartP = GetGridLocation(A);
        
        v2i EndP = GetGridLocation(B);
        
        world_chunk *StartChunk = GetOrCreateWorldChunk(GameState->Chunks, StartP.X, StartP.Y, 0, ChunkNull);
        StartChunk->Type = StartChunk->MainType;
        //Assert(StartChunk->Type == ChunkEntity);
        
        
        world_chunk *EndChunk = GetOrCreateWorldChunk(GameState->Chunks, EndP.X, EndP.Y, 0, ChunkNull);
        
        EndChunk->Type = EndChunk->MainType;
        if(!IsValidChunkType(GameState, Entity, GridB)) {
            chunk_type Types[] = {ChunkLight, ChunkDark};
            
            v2i NewPos = SearchForClosestValidChunkType(GameState, GridA, Types,ArrayCount(Types));
            Entity->Pos = WorldChunkInMeters*V2i(NewPos);
        }
        
        EndChunk->Type = ChunkEntity;
        
        Entity->MoveT += dt;
        
        r32 tValue = GetFractionOfMove(Entity);
        
        //Entity->Pos = SineousLerp0To1(A, tValue, B);
        Entity->Pos = Lerp(A, tValue, B);
        
        //For Animation
        v2 Direction = Normal(B - A);
        Entity->Velocity = 20.0f*Direction;
        //
        
        if(Entity->MoveT / Entity->MovePeriod >= 1.0f) {
            Entity->Pos = B;
            Entity->MoveT -= Entity->MovePeriod;
            Entity->VectorIndexAt++;
            Entity->WentToNewMove = true;
            
            EndChunk->Type = ChunkEntity;
            
#if 1
            if(HasMovesLeft(Entity)) {
                
                v2i GoingTo = ToV2i(Entity->Path.Points[Entity->VectorIndexAt]);
                if(!IsValidChunkType(GameState, Entity, GoingTo)) {
                    Entity->Path.Count = 0;
                    Assert(!HasMovesLeft(Entity));
                    
                }
            } else {
                
            }
#endif
        }
    } else {
        if(Entity->Type == Entity_Philosopher) {
            BreakPoint;
        }
        Entity->Velocity = {};
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

inline void AddWorldChunks(game_state *GameState, u32 Count, s32 Min, s32 Max, chunk_type ChunkType) {
    
    fori_count(Count) {
        s32 Y = RandomBetween(&GameState->GeneralEntropy, Min, Max);
        s32 X = RandomBetween(&GameState->GeneralEntropy, Min, Max);
        
        world_chunk *Result = GetOrCreateWorldChunk(GameState->Chunks, X, Y, &GameState->MemoryArena, ChunkType, &GameState->ChunkFreeList);
        Assert(Result);
        
    }
    
}

inline b32 IsSolidDefault(entity_type Type) {
    b32 Result = true;
    if(Type == Entity_CheckPoint || 
       Type == Entity_Note || 
       Type == Entity_Dropper || 
       Type == Entity_Camera) {
        Result = false;
    }
    return Result;
}

inline b32 IsSolid(entity *Ent) {
    entity_type Type = Ent->Type;
    
    b32 Result = IsSolidDefault(Type);
    
    if(Type == Entity_CheckPointParent && Ent->IsOpen) {
        Result = false;
    }
    
    return Result;
}

inline entity *
AddEntity(game_state *GameState, v2 Pos, v2 Dim, entity_type EntityType, u32 ID, animation *Animation = 0) {
    
    entity *Ent = InitEntity_(GameState, Pos, Dim, EntityType, ID);
    //All entities can move of light chunks
    AddChunkType(Ent, ChunkLight);
    
    world_chunk *Chunk = GetOrCreateWorldChunk(GameState->Chunks, (s32)Pos.X, (s32)Pos.Y, 0, ChunkNull);
    if(!Chunk) {
        chunk_type ChunkType = IsSolid(Ent) ? ChunkEntity: ChunkLight;
        Chunk = GetOrCreateWorldChunk(GameState->Chunks, (s32)Pos.X, (s32)Pos.Y, &GameState->MemoryArena, ChunkType, &GameState->ChunkFreeList);
        Chunk->MainType = ChunkLight;
    }
    
    if(IsSolid(Ent)) { Chunk->Type = ChunkEntity; }
    
    if(Ent->Type != Entity_Philosopher && Ent->Type != Entity_CheckPointParent && 
       Ent->Type != Entity_CheckPoint) { //All entities except philosopher, checkpoints and checkpoint parents can exist on the dark chunk types
        AddChunkType(Ent, ChunkDark);
    }
    
    
    return Ent;
}


#define GetEntity(State, Pos, Type) GetEntity_(State, Pos, Type)
#define GetEntityAnyType(State, Pos) GetEntity_(State, Pos, Entity_Null, true, false)

internal entity *
GetEntity_(game_state *State, v2 Pos, entity_type Type, b32 TypesMatch = true, b32 MatchType = true) {
    entity *Result = 0;
    fori_count(State->EntityCount) {
        entity *Entity = State->Entities + Index;
        b32 RightType = true;
        if(MatchType) {
            RightType = (TypesMatch) ? Type == Entity->Type : Type != Entity->Type;
        }
        
        if(RightType &&(GetGridLocation(Entity->Pos) == GetGridLocation(Pos) || GetClosestGridLocation(Entity->Pos) == GetClosestGridLocation(Pos))) {
            Result = Entity;
            break;
        }
    }
    return Result;
}

inline internal rect2 GetEntityInCameraSpace(entity *Entity, v2 CamPos) {
    v2 EntityRelP = Entity->Pos - CamPos;
    
    rect2 EntityAsRect = Rect2CenterDim(EntityRelP, Entity->Dim);
    return EntityAsRect;
}

internal b32 
InitializeMove(game_state *GameState, entity *Entity, v2 TargetP_r32, b32 StrictMove = false) {
    
    r32 MetersToWorldChunks = 1.0f / WorldChunkInMeters;
    
    v2i TargetP = ToV2i_floor(MetersToWorldChunks*TargetP_r32); 
    v2i StartP = GetGridLocation(Entity->Pos);
    
    //We set the chunk type we are on to its main type just so we can walk since no entity can walk on ChunkType_entity. We set it back after it has calculated the move. Is there are better way we can do this? Oliver 6/11/17
    world_chunk *ChunkOn = GetOrCreateWorldChunk(GameState->Chunks, StartP.X, StartP.Y, 0, ChunkNull);
    Assert(ChunkOn->Type == ChunkEntity);
    ChunkOn->Type = ChunkOn->MainType;
    //
    
    if(StrictMove) {
        memory_arena *TempArena = &GameState->ScratchPad;
        temp_memory TempMem = MarkMemory(TempArena);
        search_info SearchInfo = {};
        ComputeValidArea(GameState, &SearchInfo, StartP, Entity->ValidChunkTypes, Entity->ChunkTypeCount, TempArena);
        
        world_chunk *Chunk = GetOrCreateWorldChunk(SearchInfo.VisitedHash, TargetP.X, TargetP.Y, 0, ChunkNull);
        if(Chunk) {
            //Keep TargetP the same
        } else {
            TargetP = StartP;
        }
        
        ReleaseMemory(&TempMem);
    } else{
        if(!IsValidChunkType(GameState, Entity, StartP)) {
            chunk_type Types[] = {ChunkLight, ChunkDark};
            v2i NewPos = SearchForClosestValidChunkType(GameState, StartP, Types,ArrayCount(Types));
            Entity->Pos = WorldChunkInMeters*V2i(NewPos);
            StartP = GetGridLocation(Entity->Pos);
            Assert(IsValidChunkType(GameState, Entity, StartP));
        } 
        
        TargetP = SearchForClosestValidPosition(GameState, TargetP, StartP, Entity->ValidChunkTypes, Entity->ChunkTypeCount);
    }
#if 0
    // TODO(OLIVER): Work on this
    if(TargetP == StartP) {
        // TODO(OLIVER): This might not be a valid position
        StartP = GetClosestGridLocation(Entity->Pos);
    }
#endif
    
    
    Entity->BeginOffsetTargetP = Entity->Pos- WorldChunkInMeters*ToV2(StartP); 
    Entity->EndOffsetTargetP = TargetP_r32 - WorldChunkInMeters*ToV2(TargetP);
    
    Entity->Path.Count = 0;
    
    b32 FoundTargetP =  RetrievePath(GameState, TargetP, StartP, &Entity->Path, Entity->ValidChunkTypes, Entity->ChunkTypeCount);
    if(FoundTargetP) {
        v2 * EndPoint = Entity->Path.Points + (Entity->Path.Count - 1);
        v2 *StartPoint = Entity->Path.Points;
        *StartPoint = *StartPoint + Entity->BeginOffsetTargetP;
#if END_WITH_OFFSET
        *EndPoint = *EndPoint + Entity->EndOffsetTargetP;
#endif
        Entity->VectorIndexAt = 1;
        Entity->MoveT = 0;
    }
    
    //This is us setting is back to the entity type chunk, which we changed at the start of the function
    //We don't want other entities to move onto where we are moving, kind of like multiple threads. We don't want to do this in the move function since someone could have moved to where we are moving between then an now. Instead we set the where we are moving to be an entity chunk  and don't set the ChunkOn back to an entity taken chunk. Oliver 10/11/17
    
    
    if(FoundTargetP) {
        Assert(Entity->Path.Count);
        
        v2i NextP = ToV2i(Entity->Path.Points[0]);
        world_chunk *NextChunk = GetOrCreateWorldChunk(GameState->Chunks, NextP.X, NextP.Y, 0, ChunkNull);
        Assert(NextChunk);
        //Do we want to store the chunk type on the entities if we have more than chunk types for entities?
        //
        NextChunk->Type = ChunkEntity;
    } else {
        ChunkOn->Type = ChunkEntity;
    }
    //
    
    
    return FoundTargetP;
}

internal b32 SaveLevelToDisk(game_memory *Memory, game_state *GameState, char *FileName) {
    
    game_file_handle Handle = Memory->PlatformBeginFileWrite(FileName);
    b32 WasSuccessful = false;
    temp_memory TempMem = MarkMemory(&GameState->ScratchPad);
    if(!Handle.HasErrors) {
        
        u32 TotalSize = MegaBytes(2);
        char *MemoryToWrite = (char *)PushSize(&GameState->ScratchPad, TotalSize);
        char *MemoryAt = MemoryToWrite;
        
#define GetSize() GetRemainingSize(MemoryAt, MemoryToWrite, TotalSize)
        
        ////Writing CHUNKS 
        MemoryAt += PrintS(MemoryAt, GetSize(), "//CHUNKS X Y Type MainType\n");
        
        for(u32 i = 0; i < ArrayCount(GameState->Chunks); ++i) {
            world_chunk *Chunk = GameState->Chunks[i];
            while(Chunk) {
                MemoryAt +=  PrintS(MemoryAt, GetSize(), "%d %d %d %d\n", Chunk->X, Chunk->Y, Chunk->Type, Chunk->MainType);
                
                Chunk = Chunk->Next;
            }
        }
        
        //// Writing ENTITIES 
        MemoryAt +=  PrintS(MemoryAt, GetSize(), "//ENTITIES Pos Dim Type ID CheckpointParentID\n");
        
        for(u32 i = 0; i < GameState->EntityCount; ++i) {
            entity *Entity = GameState->Entities + i;
            
            MemoryAt +=  PrintS(MemoryAt, GetSize(), "%f %f %f %f %d %d %d ", Entity->Pos.X, Entity->Pos.Y, Entity->Dim.X, Entity->Dim.Y, Entity->Type, Entity->ID, Entity->CheckPointParentID);
            
            //Add chunk types
            forN_(Entity->ChunkTypeCount, TypeIndex) {
                MemoryAt +=  PrintS(MemoryAt, GetSize(), "%d ", (u32)Entity->ValidChunkTypes[TypeIndex]);
            }
            
            MemoryAt +=  PrintS(MemoryAt, GetSize(), "\n");
            
        }
        ////Writing sound file for note 
        MemoryAt += PrintS(MemoryAt, GetSize(), "//SOUND_FILE id fileName\n");
        for(u32 i = 0; i < GameState->EntityCount; ++i) {
            entity *Entity = GameState->Entities + i;
            if(Entity->Type == Entity_CheckPoint) {
                MemoryAt +=  PrintS(MemoryAt, GetSize(), "%d %s\n", Entity->ID, Entity->SoundToPlay);
                
            }
        }
        //// Writing CheckPoints
        MemoryAt +=  PrintS(MemoryAt, GetSize(), "//CHECKPOINT_CHILDREN_OF_PARENT ID CheckpointIds\n");
        
        for(u32 i = 0; i < GameState->EntityCount; ++i) {
            entity *Entity = GameState->Entities + i;
            if(Entity->CheckPointCount) {
                MemoryAt +=  PrintS(MemoryAt, GetSize(), "%d ", Entity->ID);
                
                //Add checkpoint entity Ids 
                forN_(Entity->CheckPointCount, IDIndex) {
                    MemoryAt +=  PrintS(MemoryAt, GetSize(), "%d ", (u32)Entity->CheckPointIds[IDIndex]);
                }
                
                MemoryAt +=  PrintS(MemoryAt, GetSize(), "\n");
            }
        }
        ///////
        
        //// Writing CheckPoints
        MemoryAt +=  PrintS(MemoryAt, GetSize(), "//CHECKPOINT_PARENT_HASHTABLES ID X Y CheckPointParentID\n");
        
        for(u32 i = 0; i < GameState->EntityCount; ++i) {
            entity *Entity = GameState->Entities + i;
            if(Entity->Type == Entity_Philosopher) {
                //Add checkpoint entity Ids 
                for(s32 ChunkIndex = 0; ChunkIndex < ArrayCount(Entity->ParentCheckPointIds); ++ChunkIndex) {
                    
                    world_chunk *PhilChunk = Entity->ParentCheckPointIds[ChunkIndex];
                    
                    while(PhilChunk) {
                        
                        MemoryAt +=  PrintS(MemoryAt, GetSize(), "%d %d %d %d\n", Entity->ID, PhilChunk->X, PhilChunk->Y, (u32)PhilChunk->Type);
                        PhilChunk = PhilChunk->Next;
                    }
                }
            }
        }
        
        
        u32 MemorySize = GetStringSizeFromChar(MemoryAt, MemoryToWrite);
        Memory->PlatformWriteFile(&Handle, MemoryToWrite, MemorySize, 0);
        if(!Handle.HasErrors) {
            WasSuccessful = true;
        }
    } 
    
    ReleaseMemory(&TempMem);
    Memory->PlatformEndFile(Handle);
    
    return WasSuccessful;
}

internal void LoadLevelFile(char *FileName, game_memory *Memory, game_state *GameState) {
    game_file_handle Handle =  Memory->PlatformBeginFile(FileName);
#if !LOAD_LEVEL_FROM_FILE
    Handle.HasErrors = true;
#endif
    if(!Handle.HasErrors) {
        AddToOutBuffer("Loaded Level File \n");
        size_t FileSize = Memory->PlatformFileSize(FileName);
        temp_memory TempMem = MarkMemory(&GameState->ScratchPad);
        void *FileMemory = PushSize(&GameState->ScratchPad, FileSize);
        if(FileSize) {
            Memory->PlatformReadFile(Handle, FileMemory, FileSize, 0);
            if(!Handle.HasErrors) {
                
                enum file_data_type {
                    DATA_NULL, 
                    DATA_CHUNKS, 
                    DATA_CHECKPOINT_CHILDREN_OF_PARENT, 
                    DATA_CHECKPOINT_PARENT_HASHTABLE, 
                    DATA_ENTITIES,
                    DATA_SOUND_FILE_NAME
                };
                
                file_data_type DataType = DATA_NULL;
                
                char*At = (char *)FileMemory;
                while(*At != '\0') {
                    
                    s32 Negative = 1;
                    
                    value_data Datas[256] = {};
                    u32 DataAt = 0;
                    if(*At == '\n') {
                        At++;
                    }
                    if(*At == '\0') {
                        break;
                    }
                    while(*At != '\n' && *At != '\0') {
                        switch(*At) {
                            case ' ': {
                                At++;
                            } break;
                            case '/': {
                                At++;
                                if(*At == '/') {
                                    //These are the table names we are creating
                                    char *ChunkID = "CHUNKS";
                                    char *EntitiesID = "ENTITIES";
                                    char *CheckPointChildrenID = "CHECKPOINT_CHILDREN_OF_PARENT";
                                    char *CheckPointParentHashTables = "CHECKPOINT_PARENT_HASHTABLES";
                                    char *SoundFileName = "SOUND_FILE";
                                    At++;
                                    
                                    if(DoStringsMatch(ChunkID, At, StringLength(ChunkID))) {
                                        DataType = DATA_CHUNKS;
                                    }
                                    if(DoStringsMatch(CheckPointParentHashTables, At, StringLength(CheckPointParentHashTables))) {
                                        DataType = DATA_CHECKPOINT_PARENT_HASHTABLE;
                                    }else if(DoStringsMatch(EntitiesID, At, StringLength(EntitiesID))) {
                                        DataType = DATA_ENTITIES;
                                    }
                                    else if(DoStringsMatch(SoundFileName, At, StringLength(SoundFileName))) {
                                        DataType = DATA_SOUND_FILE_NAME;
                                    }
                                    else if(DoStringsMatch(CheckPointChildrenID, At, StringLength(CheckPointChildrenID))) {
                                        DataType = DATA_CHECKPOINT_CHILDREN_OF_PARENT;
                                    }
                                    
                                    while(*At != '\n' && *At != '\0') {
                                        At++;
                                    }
                                    if(*At == '\n') {
                                        At++;
                                    }
                                    
                                }
                            } break;
                            case '-': {
                                Negative = -1;
                                At++;
                            } break;
                            default: {
                                value_data *Data = Datas + DataAt++;
                                if(IsNumeric(*At)) {
                                    char *AtStart = At;
                                    b32 IsFloat = false;
                                    while(IsNumeric(*At)) {
                                        if(*At == '.') {
                                            IsFloat = true;
                                        } 
                                        
                                        At++;
                                    }
                                    s32 Length = (s32)(At - AtStart);
                                    
                                    if(IsFloat) {
                                        Data->Type = VALUE_FLOAT;
                                        
                                        r32 *A = (r32 *)&Data->Value;
                                        *A = Negative*StringToFloat(AtStart, Length);
                                    } else {
                                        Data->Type = VALUE_INT;
                                        s32 *A = (s32 *)&Data->Value;
                                        *A = Negative*StringToInteger(AtStart, Length);
                                    }
                                    
                                    Negative = 1;
                                    
                                } else if(IsAlphaNumeric(*At)) {
                                    //This is string values. We have to create a buffer here. 
                                    
                                    u32 Count = 0;
                                    char Text[256] = {};
                                    while(IsAlphaNumeric(*At) || IsNumeric(*At)) {
                                        Text[Count++] = *At;
                                        At++;
                                    }
                                    Text[Count++] = '\0';
                                    char *Buffer = PushArray(&GameState->StringArena, char, Count);
                                    CopyStringToBuffer(Buffer, Count, Text);
                                    
                                    Data->Type = VALUE_STRING;
                                    char** A = (char **)&Data->Value;
                                    *A = Buffer;
                                } else {
                                    InvalidCodePath;
                                }
                            }
                            
                        }
                        
                    }
                    
                    Assert(DataType != DATA_NULL);
                    
                    if(DataAt) {
                        switch(DataType) {
                            case (DATA_CHUNKS): {
                                Assert(DataAt == 4);
                                world_chunk *Chunk = GetOrCreateWorldChunk(GameState->Chunks, CastAs(s32, Datas[0].Value), CastAs(s32, Datas[1].Value), &GameState->MemoryArena, (chunk_type)CastAs(s32, Datas[3].Value), &GameState->ChunkFreeList);
                                //Chunk->Type = (chunk_type)CastAs(s32, Datas[2].Value);
                                
                            } break;
                            case(DATA_SOUND_FILE_NAME): {
                                Assert(DataAt == 2);
                                u32 EntID = CastAs(u32, Datas[0].Value);char *SoundName = CastAs(char *, Datas[1].Value);
                                Assert(Datas[1].Type == VALUE_STRING);
                                entity *Ent = FindEntityFromID(GameState, EntID);
                                Assert(Ent);
                                Ent->SoundToPlay = SoundName;
                            } break;
                            case(DATA_CHECKPOINT_PARENT_HASHTABLE): {
                                Assert(DataAt == 4);
                                
                                u32 EntID = CastAs(u32, Datas[0].Value);s32 X = CastAs(s32, Datas[1].Value);
                                s32 Y = CastAs(s32, Datas[2].Value);
                                u32 ParentID = CastAs(s32, Datas[3].Value);
                                
                                entity *Ent = FindEntityFromID(GameState, EntID);
                                Assert(Ent);
                                //Maybe we just want to if this to handle deprecated data? Oliver 6/11/17
                                
                                GetOrCreateWorldChunk(Ent->ParentCheckPointIds, X, Y, &GameState->MemoryArena, (chunk_type)ParentID, &GameState->ChunkFreeList);
                                
                            } break;
                            case(DATA_ENTITIES): {
                                u32 EntityInfoCount = 7;
                                Assert(DataAt >= EntityInfoCount);
                                v2 Pos = V2(CastAs(r32, Datas[0].Value), CastAs(r32, Datas[1].Value));
                                v2 Dim = V2(CastAs(r32, Datas[2].Value), CastAs(r32, Datas[3].Value));
                                entity_type Type = (entity_type)CastAs(s32, Datas[4].Value);
                                u32 ID = CastAs(u32, Datas[5].Value);
                                //This value is just relevant to the checkpoint children, but saves us having to create another table!
                                u32 CheckpointParentID = CastAs(u32, Datas[6].Value);
                                
                                
                                Assert(ID > 0);
                                if(GameState->EntityIDAt <= ID) {
                                    GameState->EntityIDAt = ID + 1;
                                }
                                
                                if(Type == Entity_Player) {
                                    GameState->RestartPlayerPosition = Pos;
                                }
                                
                                entity *Entity = AddEntity(GameState, WorldChunkInMeters*GetGridLocationR32(Pos), Dim, Type, ID);
                                Entity->CheckPointParentID = CheckpointParentID;
                                
                                for(u32 ValueIndex = EntityInfoCount; ValueIndex < DataAt; ++ValueIndex) {AddChunkType(Entity, (chunk_type)CastAs(s32, Datas[ValueIndex].Value));
                                }
                            } break;
                            case(DATA_CHECKPOINT_CHILDREN_OF_PARENT): {
                                u32 EntityInfoCount = 1;
                                Assert(DataAt >= EntityInfoCount);
                                
                                u32 ID = CastAs(u32, Datas[0].Value);
                                
                                entity *Entity = FindEntityFromID(GameState, ID);
                                Assert(Entity->Type == Entity_CheckPointParent);
                                for(u32 ValueIndex = EntityInfoCount; ValueIndex < DataAt; ++ValueIndex) {
                                    AddCheckPointIDToCheckPointParent(Entity,
                                                                      CastAs(u32, Datas[ValueIndex].Value));
                                }
                            } break;
                            default: {
                                InvalidCodePath;
                            }
                        }
                    }
                }
            }
        }
        ReleaseMemory(&TempMem);
        
    } else {
        GameState->EntityIDAt = 1; //Use null ID as Null reference
        entity *Player = AddEntity(GameState, V2(-1, -1), V2(WorldChunkInMeters, WorldChunkInMeters), Entity_Player, GameState->EntityIDAt++);
        
        GameState->RestartPlayerPosition = V2(-1, -1);
        
        AddChunkType(Player, ChunkDark);
        AddChunkType(Player, ChunkLight);
        
        entity *Camera = AddEntity(GameState, V2(0, 0), V2(0, 0), Entity_Camera, GameState->EntityIDAt++);
        Camera->Collides = false;
        
        
#if CREATE_PHILOSOPHER
        entity *Philosopher = AddEntity(GameState, V2(2, 2), V2(1, 1), Entity_Philosopher, GameState->EntityIDAt++);
        Philosopher->MovePeriod = 0.4f;
        AddChunkType(Philosopher, ChunkLight);
        
        v2i PhilosopherPos = GetGridLocation(Philosopher->Pos);
        GetOrCreateWorldChunk(GameState->Chunks, PhilosopherPos.X, PhilosopherPos.Y, &GameState->MemoryArena, ChunkLight);
#endif
        
        AddWorldChunks(GameState, 200, 0, 100, ChunkLight);
        AddWorldChunks(GameState, 200, -100, 0, ChunkDark);
        
        //Create a whole load of blocks
        v2 Pos = {};
        fori_count(10) {
            do {
                Pos.X = (r32)RandomBetween(&GameState->GeneralEntropy, -10, 10);
                Pos.Y = (r32)RandomBetween(&GameState->GeneralEntropy, -10, 10);
            } while(Pos == Player->Pos 
                    #if CREATE_PHILOSOPHER
                    || Pos == Philosopher->Pos
                    #endif
                    );
            //AddEntity(GameState, Pos, Entity_Block);
        }
        
        
        // NOTE(OLIVER): Make sure player is on a valid tile
        v2i PlayerPos = GetGridLocation(Player->Pos);
        GetOrCreateWorldChunk(GameState->Chunks, PlayerPos.X, PlayerPos.Y, &GameState->MemoryArena, ChunkDark, &GameState->ChunkFreeList);
        
    }
    
    Memory->PlatformEndFile(Handle);
    
}

internal void
GameUpdateAndRender(bitmap *Buffer, game_memory *Memory, render_group *OrthoRenderGroup, render_group *RenderGroup,  r32 dt)
{
    game_state *GameState = (game_state *)Memory->GameStorage;
    r32 MetersToPixels = 60.0f;
    entity *Player = 0;
    if(!GameState->IsInitialized)
    {
        //for stb_image so the images aren't upside down.
        stbi_set_flip_vertically_on_load(true);
        //
        
        Assert(sizeof(game_state) < Memory->GameStorageSize);
        InitializeMemoryArena(&GameState->MemoryArena, (u8 *)Memory->GameStorage + sizeof(game_state), Memory->GameStorageSize - sizeof(game_state));
        
        
        
        GameState->PerFrameArena = SubMemoryArena(&GameState->MemoryArena, MegaBytes(2));
        
        GameState->ScratchPad = SubMemoryArena(&GameState->MemoryArena, MegaBytes(2));
        
        GameState->RenderArena = SubMemoryArena(&GameState->MemoryArena, MegaBytes(2));
        
        GameState->StringArena = SubMemoryArena(&GameState->MemoryArena, MegaBytes(8));
        GameState->TransientArena = SubMemoryArena(&GameState->MemoryArena, MegaBytes(3));
        
        GameState->AudioState = {};
        GameState->AudioState.PermArena = &GameState->TransientArena;
        
        for(u32 i = 0; i < ArrayCount(GameState->ThreadArenas); ++i) {
            GameState->ThreadArenas[i] = SubMemoryArena(&GameState->MemoryArena, KiloBytes(1));
        }
        
        GameState->UIState = PushStruct(&GameState->MemoryArena, ui_state);
        
        GameState->UIState->TransientTextBoxArena = SubMemoryArena(&GameState->MemoryArena, MegaBytes(1));
        
        GameState->UIState->StaticTextBoxArena = SubMemoryArena(&GameState->MemoryArena, MegaBytes(1));
        
        /////////////////////////////////
        
        GameState->RenderConsole = true;
        
        GameState->MenuBackgroundMusic = LoadWavFileDEBUG(Memory, "Moonlight_Hall.wav", 0, 0, &GameState->MemoryArena);
        GameState->MenuModeSoundInstance = PlaySound(&GameState->AudioState, &GameState->MenuBackgroundMusic);
        GameState->MenuModeSoundInstance->Loop = true;
        PauseSound(&GameState->AudioState, GameState->MenuModeSoundInstance);
        
        //"Moonlight_Hall.wav","Faro.wav" podcast1.wav
        GameState->BackgroundMusic = LoadWavFileDEBUG(Memory, "mountain_wind1.wav", 0, 0, &GameState->MemoryArena);
        GameState->OpenSound = LoadWavFileDEBUG(Memory, "openSound1.wav", 0, 0, &GameState->MemoryArena);
        GameState->BackgroundSoundInstance = PlaySound(&GameState->AudioState, &GameState->BackgroundMusic);
        GameState->BackgroundSoundInstance->Loop = true;
        
        GameState->PushSound[GameState->PushSoundCount++] = LoadWavFileDEBUG(Memory, "CNOtes1.wav", 0, 0, &GameState->MemoryArena);
        
        GameState->PushSound[GameState->PushSoundCount++] = LoadWavFileDEBUG(Memory, "GNOtes1.wav", 0, 0, &GameState->MemoryArena);
        
#if 0
        //PushSound(GameState, &GameState->BackgroundMusic, 1.0, false);
        GameState->FootstepsSound[0] = LoadWavFileDEBUG(Memory, "foot_steps_sand1.wav", &GameState->MemoryArena);
        
        GameState->FootstepsSound[1] = LoadWavFileDEBUG(Memory, "foot_steps_sand2.wav", &GameState->MemoryArena);
        
        
        GameState->MossBlockBitmap = LoadBitmap(Memory, 0, "moss_block1.bmp");
#endif
        //GameState->MagicianHandBitmap = LoadBitmap(Memory, 0, "magician_hand.bmp");
        
        GameState->DarkTiles[NULL_TILE] = LoadBitmap(Memory, 0, "static_center.bmp");
        
        GameState->DarkTiles[TOP_LEFT_TILE] = LoadBitmap(Memory, 0, "static_left.bmp");
        GameState->DarkTiles[TOP_CENTER_TILE] = LoadBitmap(Memory, 0, "static_up.bmp");
        GameState->DarkTiles[TOP_RIGHT_TILE] = LoadBitmap(Memory, 0, "static_right.bmp");
        
        GameState->DarkTiles[CENTER_LEFT_TILE] = LoadBitmap(Memory, 0, "static_left.bmp");
        GameState->DarkTiles[CENTER_TILE] = LoadBitmap(Memory, 0, "static_center.bmp");
        GameState->DarkTiles[CENTER_RIGHT_TILE] = LoadBitmap(Memory, 0, "static_right.bmp");
        
        GameState->DarkTiles[BOTTOM_LEFT_TILE] = LoadBitmap(Memory, 0, "static_left.bmp");
        GameState->DarkTiles[BOTTOM_CENTER_TILE] = LoadBitmap(Memory, 0, "static_bottom.bmp");
        GameState->DarkTiles[BOTTOM_RIGHT_TILE] = LoadBitmap(Memory, 0, "static_right.bmp");
        
        //GameState->Crate = LoadImage(Memory, &GameState->MemoryArena, "crate.bmp ", V2(0.5f, 0.3f));
        GameState->Crate = LoadBitmap(Memory, 0, "door4.bmp", V2(0.5f, 0.3f));
        
        GameState->Water = LoadBitmap(Memory, 0, "water3.bmp");
        GameState->Desert = LoadBitmap(Memory, 0, "desert.bmp");
        
        GameState->Door = LoadBitmap(Memory, 0, "door4.bmp", V2(0.5f, 0.05f));
        
        GameState->Monster = LoadBitmap(Memory, 0, "flowers/flower.bmp");
        
        //background_sun
        GameState->BackgroundImage = LoadImage(Memory, &GameState->MemoryArena, "sky_background.png", V2(0.5f, 0.5f));
        
        
        GameState->KnightAnimationCount = 0;
        
        char *BaseName = "knight/knight iso char_";
        //char *BaseName = "knight iso char_";
        
        
#define FILE_EXTENSION ".png"
#define CREATE_NAME(Append) CREATE_NAME_(#Append)
#define CREATE_NAME_(Append) Concat(&GameState->StringArena, BaseName, Concat(&GameState->StringArena, Append, FILE_EXTENSION))
        
        {
            char *FileNames[] = {
                CREATE_NAME(idle_0),
                CREATE_NAME(idle_1),
                CREATE_NAME(idle_2),
                CREATE_NAME(idle_3),
            };
            InitAnimation(&GameState->KnightAnimations[GameState->KnightAnimationCount++], Memory, FileNames, ArrayCount(FileNames), 0.75f*TAU32, "Knight_Idle", &GameState->MemoryArena);
        }
        
        {
            char *FileNames[] = {
                CREATE_NAME(run down_0),
                CREATE_NAME(run down_1),
                CREATE_NAME(run down_2),
                CREATE_NAME(run down_3),
                CREATE_NAME(run down_4)
            };
            
            InitAnimation(&GameState->KnightAnimations[GameState->KnightAnimationCount++], Memory, FileNames, ArrayCount(FileNames), 0.75f*TAU32, "Knight_Run_Down", &GameState->MemoryArena);
        }
        {
            char *FileNames[] = {
                CREATE_NAME(run up_0),
                CREATE_NAME(run up_1),
                CREATE_NAME(run up_2),
                CREATE_NAME(run up_3),
                CREATE_NAME(run up_4),
            };
            
            InitAnimation(&GameState->KnightAnimations[GameState->KnightAnimationCount++], Memory, FileNames, ArrayCount(FileNames), 0.25f*TAU32, "Knight_Run_Up", &GameState->MemoryArena);
        }
        {
            char *FileNames[] = {
                CREATE_NAME(run left_0),
                CREATE_NAME(run left_1),
                CREATE_NAME(run left_2),
                CREATE_NAME(run left_3),
                CREATE_NAME(run left_4),
                CREATE_NAME(run left_5),
            };
            
            InitAnimation(&GameState->KnightAnimations[GameState->KnightAnimationCount++], Memory, FileNames, ArrayCount(FileNames), 0.5f*TAU32, "Knight_Run_Left", &GameState->MemoryArena);
        }
        {
            char *FileNames[] = {
                CREATE_NAME(run right_0),
                CREATE_NAME(run right_1),
                CREATE_NAME(run right_2),
                CREATE_NAME(run right_3),
                CREATE_NAME(run right_4),
                CREATE_NAME(run right_5),
            };
            
            InitAnimation(&GameState->KnightAnimations[GameState->KnightAnimationCount++], Memory, FileNames, ArrayCount(FileNames), 0*TAU32, "Knight_Run_Right", &GameState->MemoryArena);
        }
        BaseName = "chubby_iso_character/idle_";
        {
            char *FileNames[] = {
                CREATE_NAME(idle_0),
                CREATE_NAME(idle_1),
                CREATE_NAME(idle_2),
                CREATE_NAME(idle_3),
            };
            InitAnimation(&GameState->ManAnimations[GameState->ManAnimationCount++], Memory, FileNames, ArrayCount(FileNames), 0.75f*TAU32, "Man_Idle", &GameState->MemoryArena);
        }
        
        {
            char *FileNames[] = {
                CREATE_NAME(walk forward_0),
                CREATE_NAME(walk forward_1),
                CREATE_NAME(walk forward_2),
                CREATE_NAME(walk forward_3)
            };
            
            InitAnimation(&GameState->ManAnimations[GameState->ManAnimationCount++], Memory, FileNames, ArrayCount(FileNames), 0.75f*TAU32, "Man_Run_Down", &GameState->MemoryArena);
        }
        {
            char *FileNames[] = {
                CREATE_NAME(walk up_0),
                CREATE_NAME(walk up_1),
                CREATE_NAME(walk up_2)
            };
            
            InitAnimation(&GameState->ManAnimations[GameState->ManAnimationCount++], Memory, FileNames, ArrayCount(FileNames), 0.25f*TAU32, "Man_Run_Up", &GameState->MemoryArena);
        }
        {
            char *FileNames[] = {
                CREATE_NAME(walk left_0),
                CREATE_NAME(walk left_1),
                CREATE_NAME(walk left_2),
                CREATE_NAME(walk left_3)
            };
            
            InitAnimation(&GameState->ManAnimations[GameState->ManAnimationCount++], Memory, FileNames, ArrayCount(FileNames), 0.5f*TAU32, "Man_Run_Left", &GameState->MemoryArena);
        }
        {
            char *FileNames[] = {
                CREATE_NAME(walk right_0),
                CREATE_NAME(walk right_1),
                CREATE_NAME(walk right_2),
                CREATE_NAME(walk right_3)
            };
            
            InitAnimation(&GameState->ManAnimations[GameState->ManAnimationCount++], Memory, FileNames, ArrayCount(FileNames), 0*TAU32, "Man_Run_Right", &GameState->MemoryArena);
        }
        
        GameState->GameMode = PLAY_MODE;
        
        GameState->Fonts = LoadFontFile("fonts.clm", Memory, &GameState->MemoryArena);
        
        font_quality_value Qualities[FontQuality_Count] = {};
        Qualities[FontQuality_Debug] = InitFontQualityValue(1.0f);
        Qualities[FontQuality_Size] = InitFontQualityValue(1.0f);
        
        //GameState->Font = FindFont(GameState->Fonts, Qualities);
        GameState->DebugFont = FindFontByName(GameState->Fonts, "Liberation Mono", Qualities);
        GameState->GameFont = FindFontByName(GameState->Fonts, "Karmina Regular", Qualities);
        Memory->DebugFont = GameState->DebugFont;
        InitConsole(&DebugConsole, GameState->DebugFont, 0, 2.0f, &GameState->MemoryArena);
        
        
        GameState->PauseMenu.Timer.Period = 2.0f;
        GameState->PauseMenu.Font = GameState->GameFont;
        GameState->GameOverMenu.Timer.Period = 2.0f;
        GameState->GameOverMenu.Font = GameState->GameFont;
        
        // NOTE(OLIVER): Create Board
        
        LoadLevelFile("level1.omm", Memory, GameState);
        
        ///////////////
#if 1
        ui_element_settings UISet = {};
        UISet.Pos = V2(0.7f*(r32)Buffer->Width, 20);
        UISet.Name = 0;
        
        PushUIElement(GameState->UIState, UI_Moveable,  UISet);
        {
            UISet.Type = UISet.Name = "Save Level";
            AddUIElement(GameState->UIState, UI_Button, UISet);
            
            UISet.Type = UISet.Name = "Clear Introspect Window";
            AddUIElement(GameState->UIState, UI_Button, UISet);
            
            UISet.ValueLinkedToPtr = &GameState->RenderMainChunkType;
            UISet.Name = "Render Main Chunk Type";
            AddUIElement(GameState->UIState, UI_CheckBox, UISet);
        }
        PopUIElement(GameState->UIState);
        
        UISet = {};
        UISet.Pos = V2(0.4f*(r32)Buffer->Width, 20);
        UISet.Name = 0;
        
        
        PushUIElement(GameState->UIState, UI_Moveable,  UISet);
        {
            memory_arena *StaticArena = &GameState->UIState->StaticTextBoxArena;
            InitCharBuffer(&UISet.Buffer, 256, StaticArena, StaticArena);
            UISet.Type = "u32";
            UISet.ValueLinkedToPtr = &GameState->UIState->PhilosopherID;
            UISet.Name = "Philosopher ID:  ";
            
            AddUIElement(GameState->UIState, UI_TextBox, UISet);
            
            InitCharBuffer(&UISet.Buffer, 256, StaticArena, StaticArena);
            UISet.Type = "u32";
            UISet.ValueLinkedToPtr = &GameState->UIState->CheckPointParentID;
            UISet.Name = "Check Point Parent ID:  ";
            
            AddUIElement(GameState->UIState, UI_TextBox, UISet);
        }
        PopUIElement(GameState->UIState);
        
        
        UISet= {};
        UISet.Pos = V2(0, 20);
        
        SetEditorMode(GameState->UIState, SELECT_MODE);
        
        SetEntityTypeMode(GameState->UIState, Entity_Block);
        
        PushUIElement(GameState->UIState, UI_Moveable,  UISet);
        {
            UISet.Active = true;
            u32 DropDownBoxParentIndex = PushUIElement(GameState->UIState, UI_DropDownBoxParent, UISet);
            ui_element *Elm =GameState->UIState->Elements + DropDownBoxParentIndex;
            Elm->Set.ValueLinkedToPtr = &GameState->UIState->InitInfo;
            Elm->Set.EnumArray.Array = entity_type_Names;
            
            UISet.ValueLinkedToPtr = &Elm->Set.ScrollAt;
            AddUIElement(GameState->UIState, UI_Slider, UISet);
            {
                for(u32 TypeI = 0; TypeI < ArrayCount(entity_type_Values); ++TypeI) {
                    UISet.Name = entity_type_Names[TypeI];
                    UISet.EnumArray.Array = entity_type_Values;
                    UISet.EnumArray.Index = TypeI;
                    UISet.EnumArray.Type = entity_type_Names[ArrayCount(entity_type_Names) - 1];
                    UISet.ValueLinkedToPtr = &GameState->UIState->InitInfo;
                    
                    AddUIElement(GameState->UIState, UI_DropDownBox, UISet);
                }
                
                for(u32 TypeI = 0; TypeI < ArrayCount(chunk_type_Values); ++TypeI) {
                    UISet.Name = chunk_type_Names[TypeI];
                    UISet.EnumArray.Array = chunk_type_Values;
                    UISet.EnumArray.Index = TypeI;
                    UISet.EnumArray.Type = chunk_type_Names[ArrayCount(chunk_type_Names) - 1];
                    UISet.ValueLinkedToPtr = &GameState->UIState->InitInfo;
                    
                    AddUIElement(GameState->UIState, UI_DropDownBox, UISet);
                }
                
            }
            PopUIElement(GameState->UIState);
            
            DropDownBoxParentIndex = PushUIElement(GameState->UIState, UI_DropDownBoxParent, UISet);
            {
                Elm = GameState->UIState->Elements + DropDownBoxParentIndex;
                Elm->Set.ValueLinkedToPtr = &GameState->UIState->EditorMode;
                Elm->Set.EnumArray.Array = editor_mode_Names;
                
                UISet.ValueLinkedToPtr = &Elm->Set.ScrollAt;
                AddUIElement(GameState->UIState, UI_Slider, UISet);
                {
                    for(u32 TypeI = 0; TypeI < ArrayCount(editor_mode_Values); ++TypeI) {
                        UISet.Name = editor_mode_Names[TypeI];
                        UISet.EnumArray.Array = editor_mode_Values;
                        UISet.EnumArray.Index = TypeI;
                        UISet.EnumArray.Type = editor_mode_Names[ArrayCount(editor_mode_Names) - 1];
                        UISet.ValueLinkedToPtr = &GameState->UIState->EditorMode;
                        
                        AddUIElement(GameState->UIState, UI_DropDownBox, UISet);
                    }
                }
            }
            PopUIElement(GameState->UIState);
            
            UISet.ValueLinkedToPtr = &GameState->UIState->ControlCamera;
            UISet.Name = "Control Camera";
            AddUIElement(GameState->UIState, UI_CheckBox, UISet);
            
            UISet.ValueLinkedToPtr = &GameState->UIState->GamePaused;
            UISet.Name = "Pause Game";
            AddUIElement(GameState->UIState, UI_CheckBox, UISet);
        }
        PopUIElement(GameState->UIState);
#endif
        GameState->IsInitialized = true;
        
        Player = FindFirstEntityOfType(GameState, Entity_Player);
        
        if(Player) {
            
            AddAnimationToList(GameState, &GameState->MemoryArena, Player, FindAnimation(GameState->KnightAnimations, GameState->KnightAnimationCount, "Knight_Idle"));
        } 
        
        
        
        entity *Phil = FindFirstEntityOfType(GameState, Entity_Philosopher);
        
        if(Phil) {
            Assert(Phil);
            
            AddAnimationToList(GameState, &GameState->MemoryArena, Phil, FindAnimation(GameState->ManAnimations, GameState->ManAnimationCount, "Man_Idle"));
        }
        
    }
    EmptyMemoryArena(&GameState->RenderArena);
    
    //This flips the axis to top-down on the ortho render group
    mat2 rotMat = Mat2();
    rotMat.B.Y = -1;
    
    InitRenderGroup(OrthoRenderGroup, Buffer, &GameState->RenderArena, MegaBytes(1), V2(1, 1), V2i(0, Buffer->Height), rotMat, Memory->ThreadInfo, GameState, Memory);
    
    InitRenderGroup(RenderGroup, Buffer, &GameState->RenderArena, MegaBytes(1), V2(60, 60), 0.5f*V2i(Buffer->Width, Buffer->Height), Mat2(), Memory->ThreadInfo, GameState, Memory);
    
    temp_memory PerFrameMemory = MarkMemory(&GameState->PerFrameArena);
    
    DebugConsole.RenderGroup = OrthoRenderGroup;
    
    rect2 BufferRect = Rect2(0, 0, (r32)Buffer->Width, (r32)Buffer->Height);
    
    v2 BufferSize = 0.5f*BufferRect.Max;
    
    PushClear(RenderGroup, V4(0.5f, 0.5f, 0, 1));
    
    PushBitmap(RenderGroup, V3(0, 0, 1), &GameState->BackgroundImage, BufferRect.Max.X/MetersToPixels, BufferRect);
    // TODO(Oliver): I guess it doesn't matter but maybe push this to the ortho group once z-index scheme is in place. 
    
    Player = FindFirstEntityOfType(GameState, Entity_Player);
    
    
    switch(GameState->GameMode) {
        case MENU_MODE: {
            if(WasPressed(Memory->GameButtons[Button_Escape])) {
                PauseSound(&GameState->AudioState, GameState->MenuModeSoundInstance);
                PlaySound(&GameState->AudioState, GameState->BackgroundSoundInstance);
                GameState->GameMode = PLAY_MODE;
                break;
            }
            UpdateMenu(&GameState->PauseMenu,GameState, Memory, OrthoRenderGroup, dt);
            GameState->RenderConsole = false;
            
        } break;
        case GAMEOVER_MODE: {
            
            char *MenuOptions[] = {"Restart", "Quit"};
            
            if(WasPressed(Memory->GameButtons[Button_Enter])) {
                switch(GameState->GameOverMenu.IndexAt) {
                    case 0: {
                        Player->Pos = GameState->RestartPlayerPosition;
                        Player->VectorIndexAt = Player->Path.Count;
                        GameState->GameMode = PLAY_MODE;
                    } break;
                    case 1: {
                        EndProgram(Memory); 
                    } break;
                }
                GameState->GameOverMenu.Timer.Value = 0.0f;
            }
            
            UpdateAndRenderMenu(&GameState->GameOverMenu, OrthoRenderGroup, MenuOptions, ArrayCount(MenuOptions), dt, Memory);
        } break;
        case WIN_MODE: {
            char *MenuOptions[] = {"Restart", "Quit"};
            //Go to next level here?
            if(WasPressed(Memory->GameButtons[Button_Enter])) {
                switch(GameState->GameOverMenu.IndexAt) {
                    case 0: {
                        Player->Pos = V2(-1, -1);
                        Player->VectorIndexAt = Player->Path.Count;
                        GameState->GameMode = PLAY_MODE;
                    } break;
                    case 1: {
                        EndProgram(Memory); 
                    } break;
                }
                GameState->GameOverMenu.Timer.Value = 0.0f;
            }
            
            UpdateAndRenderMenu(&GameState->GameOverMenu, OrthoRenderGroup, MenuOptions, ArrayCount(MenuOptions), dt, Memory);
        }
        case PLAY_MODE: {
            GameState->RenderConsole = true;
            
            v2 MouseP_PerspectiveSpace = InverseTransform(&RenderGroup->Transform, V2(Memory->MouseX, Memory->MouseY));
            v2 MouseP_OrthoSpace = InverseTransform(&OrthoRenderGroup->Transform, V2(Memory->MouseX, Memory->MouseY));
            v2i GlobalPlayerMove = {};
            
            if(WasPressed(Memory->GameButtons[Button_Escape]))
            {
                PlaySound(&GameState->AudioState, GameState->MenuModeSoundInstance);
                PauseSound(&GameState->AudioState, GameState->BackgroundSoundInstance);
                GameState->GameMode = MENU_MODE;
                break;
            }
            if(WasPressed(Memory->GameButtons[Button_F3])) {
                Flip(GameState->ShowHUD);
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
                if(InBounds(DebugConsole.InputConsoleRect, (s32)Memory->MouseX, (s32)Memory->MouseY)) {
                    DebugConsole.InputIsHot = true;
                }
            }
            if(WasReleased(Memory->GameButtons[Button_LeftMouse])) {
                
                if(InBounds(DebugConsole.InputConsoleRect, (s32)Memory->MouseX, (s32)Memory->MouseY) && DebugConsole.InputIsHot) {
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
                for(u32 KeyIndex = 0; KeyIndex <= Memory->SizeOfGameKeys; ++KeyIndex) {
                    
                    game_button *Button = Memory->GameKeys + KeyIndex;
                    
                    if(Button->IsDown) {
                        DebugConsole.InputTimer.Value = 0;
                        switch(KeyIndex) {
                            case 8: { //Backspace
                                Splice_("", &DebugConsole.Input, -1);
                            } break;
                            case 13: { //Enter
                                //Don't do anything
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
                
            } else if (GameState->UIState->InteractingWith && GameState->UIState->InteractingWith->Type == UI_TextBox) {
                
                ui_state *UIState = GameState->UIState;
                
                
                ui_element *Elm = UIState->InteractingWith;
                
                char_buffer *ElmBuffer = &Elm->Set.Buffer;
                if(WasPressed(Memory->GameButtons[Button_Arrow_Left])) {
                    ElmBuffer->WriteIndexAt = Max(0, (s32)ElmBuffer->WriteIndexAt - 1);
                    //DebugConsole.InputTimer.Value = 0;
                }
                if(WasPressed(Memory->GameButtons[Button_Arrow_Right])) {
                    ElmBuffer->WriteIndexAt = Min(ElmBuffer->IndexAt, ElmBuffer->WriteIndexAt + 1);
                    //DebugConsole.InputTimer.Value = 0;
                }
                
                for(u32 KeyIndex = 0; KeyIndex <= Memory->SizeOfGameKeys; ++KeyIndex) {
                    
                    game_button *Button = Memory->GameKeys + KeyIndex;
                    
                    if(Button->IsDown) {
                        //DebugConsole.InputTimer.Value = 0;
                        switch(KeyIndex) {
                            case 8: { //Backspace
                                Splice_("", ElmBuffer, -1);
                            } break;
                            case 13: { //Enter
                                //Don't do anything
                            } break;
                            default: {
                                AddToBufferGeneral(ElmBuffer, "%s", &KeyIndex);
                            }
                        }
                    }
                }
                
                
                if(WasPressed(Memory->GameButtons[Button_Enter])) {
                    //Finished with editing the box
                    //We have to now parse the data here!
                    
                    if(DoStringsMatch(Elm->Set.Type, "u32") || 
                       DoStringsMatch(Elm->Set.Type, "s32")) {
                        s32 Value = StringToInteger(ElmBuffer->Chars, ElmBuffer->IndexAt);
                        *((s32 *)Elm->Set.ValueLinkedToPtr) = Value;
                    } else if(DoStringsMatch(Elm->Set.Type, "r32")) {
                        r32 Value = StringToFloat(ElmBuffer->Chars, ElmBuffer->IndexAt);
                        *((r32 *)Elm->Set.ValueLinkedToPtr) = Value;
                    } else if(DoStringsMatch(Elm->Set.Type, "char *")) {
                        //This is a memory leak since we never clear the unused text buffers. We need more of a dynamic memory allocator. 
                        char *TextBuffer = PushArray(&GameState->StringArena, char, ElmBuffer->IndexAt);
                        CopyStringToBuffer(TextBuffer, ElmBuffer->IndexAt, ElmBuffer->Chars);
                        *((char **)Elm->Set.ValueLinkedToPtr) = TextBuffer;
                    } else {
                        AddToOutBuffer("Type not recognized.");
                    }
                    
                    
                    ClearBuffer(ElmBuffer);
                    UIState->InteractingWith = 0;
                }
                
            } else {
                
                v2i PlayerGridP = {};
                v2i PlayerMove = {};
                
                if(CanMove(Player)) {  // NOTE(Oliver): This is to stop the player moving diagonal.  
                    if(PLAYER_MOVE_ACTION(Memory->GameButtons[Button_Left]))
                    {
                        PlayerMove = V2int(-1, 0);
                        PlayerGridP = GetGridLocation(Player->Pos);
                    }
                    else if(PLAYER_MOVE_ACTION(Memory->GameButtons[Button_Right]))
                    {
                        PlayerMove = V2int(1, 0);
                        PlayerGridP = GetClosestGridLocation(Player->Pos);
                    }
                    else if(PLAYER_MOVE_ACTION(Memory->GameButtons[Button_Down]))
                    {
                        PlayerMove = V2int(0, -1);
                        PlayerGridP = GetGridLocation(Player->Pos);
                    }
                    else if(PLAYER_MOVE_ACTION(Memory->GameButtons[Button_Up]))
                    {
                        PlayerMove = V2int(0, 1);
                        PlayerGridP = GetClosestGridLocation(Player->Pos);
                    }
                }
                
                if(DEFINE_MOVE_ACTION(Memory->GameButtons[Button_Space]))
                {
                    if(GameState->PlayerIsSettingPath) {
                        PlayerGridP = GetGridLocation(Player->Pos);
                        entity *Entity = AddEntity(GameState, V2i(PlayerGridP), V2(1, 1), Entity_Dropper, GameState->EntityIDAt++);
                        AddChunkType(Entity, ChunkLight);
                        AddChunkType(Entity, ChunkDark);
                        Entity->VectorIndexAt = 1;
                        Entity->Path = GameState->PathToSet;
                        GameState->PlayerIsSettingPath = false;
                    } else {
                        GameState->PlayerIsSettingPath = true;
                        GameState->PathToSet.Count = 0;
                        v2 PosToAdd = V2i(GetGridLocation(Player->Pos));
                        Assert(IsValidGridPosition(GameState, PosToAdd));
                        AddToPath(&GameState->PathToSet, PosToAdd);
                    }
                    
                }
                
                if(GameState->PlayerIsSettingPath) {
                    v2i DefineMove = {};
                    if(DEFINE_MOVE_ACTION(Memory->GameButtons[Button_Left]))
                    {
                        DefineMove = V2int(-1, 0);
                    }
                    else if(DEFINE_MOVE_ACTION(Memory->GameButtons[Button_Right]))
                    {
                        DefineMove = V2int(1, 0);
                        
                    }
                    else if(DEFINE_MOVE_ACTION(Memory->GameButtons[Button_Down]))
                    {
                        DefineMove = V2int(0, -1);
                        
                    }
                    else if(DEFINE_MOVE_ACTION(Memory->GameButtons[Button_Up]))
                    {
                        DefineMove = V2int(0, 1);
                        
                    }
                    
                    PlayerMove = {};
                    v2 LastMove = V2i(GetGridLocation(Player->Pos));
                    
                    if(DefineMove != V2int(0, 0)) {
                        if(GameState->PathToSet.Count > 0) {
                            LastMove =  GameState->PathToSet.Points[GameState->PathToSet.Count - 1];
                            v2 NewMove = LastMove + V2i(DefineMove);
                            if(IsValidGridPosition(GameState, NewMove)) {
                                AddToPath(&GameState->PathToSet, NewMove);
                            } else {
                                //Abort? I think this is a gameplay decision,and later code will handle whether it is valid or not.
                            }
                            
                        } 
                    }
                }
                
                if(GameState->UIState->ControlCamera || GameState->UIState->GamePaused) {
                    GlobalPlayerMove = PlayerMove;
                    PlayerMove = {};
                }
                
#if !MOVE_VIA_MOUSE 
                if(PlayerMove != V2int(0, 0)) {
                    v2i GridTargetPos = (PlayerGridP + PlayerMove);
                    world_chunk *Chunk = GetOrCreateWorldChunk(GameState->Chunks, GridTargetPos.X, GridTargetPos.Y, 0, ChunkNull);
                    
                    // NOTE(OLIVER): This is where the player moves the block.
                    entity *Block = GetEntity(GameState, WorldChunkInMeters*V2i(GridTargetPos), Entity_Block);
                    if(Chunk && Block) {
                        v2i BlockTargetPos = GridTargetPos + PlayerMove;
                        world_chunk *NextChunk = GetOrCreateWorldChunk(GameState->Chunks, BlockTargetPos.X, BlockTargetPos.Y, 0, ChunkNull);
                        
                        Assert(Block && Block->Type == Entity_Block);
                        if(NextChunk && IsValidType(NextChunk, Block->ValidChunkTypes, Block->ChunkTypeCount) && NextChunk->Type != ChunkEntity) {
                            
                            Assert(Chunk != NextChunk);
                            InitializeMove(GameState, Block, WorldChunkInMeters*V2i(BlockTargetPos));
                            if(PlayerMove.X != 0) {
                                playing_sound *SoundInstance = PlaySound(&GameState->AudioState, &GameState->PushSound[0]);
                                SoundInstance->CurrentVolumes = V2(0.6f, 0.6f);
                            } else {
                                playing_sound *SoundInstance = PlaySound(&GameState->AudioState, &GameState->PushSound[1]);
                                SoundInstance->CurrentVolumes = V2(0.6f, 0.6f);
                            }
                            
                        }
                    }
                    
                    InitializeMove(GameState, Player, WorldChunkInMeters*V2i(GridTargetPos), true);
                }
#endif
            }
            
            entity *Camera = FindFirstEntityOfType(GameState, Entity_Camera);
            
            
            
#if UPDATE_CAMERA_POS
            r32 DragFactor = 0.2f;
            v2 Acceleration = Camera->AccelFromLastFrame;
            if(GameState->UIState->ControlCamera) { //maybe turn these into defines like on Handmade Hero?
                Acceleration = 400*V2i(GlobalPlayerMove);
            } 
            Camera->Pos = dt*dt*Acceleration + dt*Camera->Velocity + Camera->Pos;
            Camera->Velocity = Camera->Velocity + dt*Acceleration - DragFactor*Camera->Velocity;
            
#endif
            
            v2 CamPos = Camera->Pos;
            
            ui_state *UIState = GameState->UIState;
#define DRAW_GRID 1
            
#if DRAW_GRID
            r32 AlphaAt = 1.0f;
            if(GameState->HideUnderWorld) {
                
            }
            r32 ScreenFactor = 0.7f;
            v2 HalfScreenDim = ScreenFactor*Hadamard(Inverse(RenderGroup->Transform.Scale), RenderGroup->ScreenDim);
            v2i Min = ToV2i(CamPos - HalfScreenDim);
            v2i Max = ToV2i(CamPos + HalfScreenDim);
            
            for(s32 X = Min.X; X < Max.X; X++) {
                for(s32 Y = Min.Y; Y < Max.Y; ++Y) {
                    
                    v2i GridP = GetGridLocation(V2i(X, Y));
                    world_chunk *Chunk = GetOrCreateWorldChunk(GameState->Chunks, GridP.X, GridP.Y, 0, ChunkNull);
                    if(Chunk) {
                        
                        r32 MinX = (Chunk->X*WorldChunkInMeters) - Camera->Pos.X;
                        r32 MinY = (Chunk->Y*WorldChunkInMeters) - Camera->Pos.Y;
                        rect2 Rect = Rect2CenterDim(V2(MinX, MinY),  V2(WorldChunkInMeters, WorldChunkInMeters));
                        
                        bitmap *Bitmap = 0;
                        v4 Color01 = {0, 0, 0, 1};
                        chunk_type Type; 
                        if(GameState->RenderMainChunkType) {
                            Type = Chunk->MainType;
                        } else {
                            Type = Chunk->Type;
                        }
                        switch (Type) {
                            case ChunkDark: {
                                Color01 = {0.2f, 0.2f, 0.2f, 1};
                                tile_pos_type PosType = GetTilePosType(GameState->Chunks, GridP.X, GridP.Y);
                                Bitmap = &GameState->DarkTiles[(u32)PosType];
                                //Bitmap = &GameState->Water;
                            } break;
                            case ChunkLight: {
                                Color01 = {0.8f, 0.8f, 0.6f, 1};
                                Bitmap = &GameState->Desert;
                            } break;
                            case ChunkEntity: {
                                Color01 = {0.7f, 0.3f, 0, 1};
                            } break;
                            case ChunkNull: {
                                Color01 = {0, 0, 0, 1};
                            } break;
                            default: {
                                InvalidCodePath;
                            }
                        }
                        
                        b32 DrawPhilosopherCheckPointChunk = false;
                        if(GameState->ShowHUD) {
                            if(UIState->EntIntrospecting && UIState->EntIntrospecting->Type == Entity_Philosopher) {
                                entity *Phil = UIState->EntIntrospecting;
                                world_chunk *PhilChunk = GetOrCreateWorldChunk(Phil->ParentCheckPointIds, GridP.X, GridP.Y, 0, ChunkNull);
                                if(PhilChunk) {
                                    DrawPhilosopherCheckPointChunk = true;
                                }
                                
                            }
                        }
                        
#if DRAW_PLAYER_PATH
                        if(IsPartOfPath(Chunk->X, Chunk->Y, &Player->Path)) {
                            Color01 = {1, 0, 1, 1};
                            PushRect(RenderGroup, Rect, 1, Color01);
                        } else 
#endif
                        if(DrawPhilosopherCheckPointChunk) {
                            Color01 = {1, 0, 1, 1};
                            PushRect(RenderGroup, Rect, 1, Color01);
                        } else {
                            if(Bitmap) {
                                PushBitmap(RenderGroup, V3(MinX, MinY, 1), Bitmap, WorldChunkInMeters, BufferRect);
                            } else {
                                PushRect(RenderGroup, Rect, 1, Color01);
                            }
                        }
                    }
                }
            }
#endif
            r32 PercentOfScreen = 0.1f;
            v2 CameraWindow = (1.0f/MetersToPixels)*PercentOfScreen*V2i(Buffer->Width, Buffer->Height);
            //NOTE(): Draw Camera bounds that the player stays within. 
            if(GameState->ShowHUD) {
                PushRectOutline(RenderGroup, Rect2(-CameraWindow.X, -CameraWindow.Y, CameraWindow.X, CameraWindow.Y), 1, V4(1, 0, 1, 1));
            }
            ////
            for(u32 EntityIndex = 0; EntityIndex < GameState->EntityCount; ++EntityIndex)
            {
                entity *Entity = GameState->Entities + EntityIndex;
                {
                    v2 EntityRelP = Entity->Pos - CamPos;
                    
                    rect2 EntityAsRect = Rect2CenterDim(EntityRelP, Entity->Dim);
                    
                    switch(Entity->Type)
                    {
                        case((u32)Entity_Player):
                        {
                            if(!GameState->UIState->GamePaused) {
                                /////////NOTE(Oliver): Update Player's postion/////////////// 
                                
                                UpdateEntityPositionViaFunction(GameState, Entity, dt);
                                if(Entity->WentToNewMove) {
                                    PlaySound(&GameState->AudioState, &GameState->FootstepsSound[Entity->WalkSoundAt++]);
                                    if(Entity->WalkSoundAt >= 2) {
                                        Entity->WalkSoundAt = 0;
                                    }
                                }
                                
                                ///////// NOTE(OLIVER): Update camera acceleration
                                if((Abs(EntityRelP.X) > CameraWindow.X || Abs(EntityRelP.Y) > CameraWindow.Y)) {
                                    Camera->AccelFromLastFrame = 10.0f*EntityRelP;
                                } else {
                                    Camera->AccelFromLastFrame = V2(0, 0);
                                }
                            }
                            /////// NOTE(OLIVER): Player Animation
                            if(!IsEmpty(&Entity->AnimationListSentintel)) {
                                
                                PushCurrentAnimationBitmap(RenderGroup, Entity, EntityRelP, BufferRect, 1.7f);
                                
                                //// NOTE(OLIVER): Preparing to look for new animation State
                                
                                animation *NewAnimation = 0;
                                r32 DirectionValue = GetDirectionInRadians(Entity);
                                
#define USE_ANIMATION_STATES 0
#if USE_ANIMATION_STATES
                                r32 Qualities[ANIMATE_QUALITY_COUNT] = {};
                                r32 Weights[ANIMATE_QUALITY_COUNT] = {1.0f};
                                
                                Qualities[DIRECTION] = DirectionValue; 
                                
                                NewAnimation = GetAnimation(GameState->KnightAnimations, GameState->KnightAnimationCount, Qualities, Weights);
#else 
                                char *AnimationName = 0;
                                r32 Speed = Length(Entity->Velocity);
                                if(Speed < 1.0f) {
                                    AnimationName = "Knight_Idle";
                                } else {
                                    if (DirectionValue == 0) {
                                        AnimationName = "Knight_Run_Right";
                                    } 
                                    else if (DirectionValue == 0.25f*TAU32) {
                                        AnimationName = "Knight_Run_Up";
                                    } 
                                    else if (DirectionValue == 0.5f*TAU32) {
                                        AnimationName = "Knight_Run_Left";
                                    } 
                                    else if (DirectionValue == 0.75f*TAU32) {
                                        AnimationName = "Knight_Run_Down";
                                    } 
                                }
                                
                                NewAnimation = FindAnimation(GameState->KnightAnimations, GameState->KnightAnimationCount, AnimationName);
#endif
                                
                                Assert(NewAnimation);
                                UpdateAnimation(GameState, Entity, dt, NewAnimation);
                                ///
                            } else {
                                
                                PushRect(RenderGroup, EntityAsRect, 1, V4(0, 1, 1, 1));
                            }
                            
                            ///////
                        } break;
                        case Entity_Note: {
                            
                            //PushBitmap(RenderGroup, V3(EntityRelP, 1), &GameState->Door,  1.1f*WorldChunkInMeters, BufferRect);
                            
                        } break;
                        case Entity_CheckPoint: {
                            entity *Philospher = GetEntity(GameState, Entity->Pos, Entity_Philosopher);
                            
                            entity *LastFrameEntity = FindEntityFromID(GameState, Entity->LastFrameEntityID);
                            if(Philospher && Philospher != LastFrameEntity) {
                                
                                Reactivate(&Entity->ParticleSystem);
                                
                                
                                loaded_sound *Sound = FindSound(GameState, Entity->SoundToPlay);
                                if(Sound) {
                                    PlaySound(&GameState->AudioState, Sound);
                                }
                                entity *Parent = FindEntityFromID(GameState, Entity->CheckPointParentID);
                                
                                b32 AllComplete = true;
                                for(u32 i = 0 ; i < Parent->CheckPointCount; ++i) {
                                    if(Parent->CheckPointIds[i] == Entity->ID) {
                                        Parent->CheckPointComplete[i] = true;
                                    }
                                    AllComplete &= Parent->CheckPointComplete[i];
                                    
                                }
                                
                                if(AllComplete && !Parent->IsOpen) {
                                    Parent->IsOpen = true;
                                    v2i ParentPos = GetGridLocation(Parent->Pos);
                                    world_chunk *ParentChunk = GetOrCreateWorldChunk(GameState->Chunks, ParentPos.X, ParentPos.Y, 0, ChunkNull);
                                    ParentChunk->Type = ParentChunk->MainType;
                                    PlaySound(&GameState->AudioState, &GameState->OpenSound);
                                    
                                }
                            }
                            
                            Entity->LastFrameEntityID = (Philospher) ? Philospher->ID : 0;
                            
                            PushRect(RenderGroup, EntityAsRect, 1, V4(0, 1, 1, 1));
                            
                            DrawParticleSystem(&Entity->ParticleSystem, RenderGroup, dt, Entity->Pos, V3(0, 1, 0), CamPos);
                            
                        } break;
                        case Entity_CheckPointParent: {
                            if(Entity->IsOpen) {
                                if(GameState->ShowHUD) {
                                    PushRect(RenderGroup, EntityAsRect, 1, V4(1, 1, 0, 1));
                                }
                            } else { 
                                PushBitmap(RenderGroup, V3(EntityRelP, 1), &GameState->Door,  0.8f*WorldChunkInMeters, BufferRect);
                            }
                            
                            
                        } break;
                        case Entity_Philosopher: {
                            if(!GameState->UIState->GamePaused) {
                                //First we try to find the player
                                v2i PlayerPos = GetGridLocation(Player->Pos);
                                world_chunk *PlayerChunk = GetOrCreateWorldChunk(GameState->Chunks, PlayerPos.X, PlayerPos.Y, 0, ChunkNull);
                                Assert(PlayerChunk);
                                b32 WasSuccessful = false;
                                
                                // NOTE(Oliver): This should be a more complete function that compares paths and decides if we need to construct a new one.
                                
                                //Case where the chunk changes to a new type, so search isn't valid anymore -> Maybe go to last valid position...
                                // TODO(Oliver): Maybe we could fix this during the move. If the chunk suddenly changes to an invalid chunk, we will try backtrace or go to the nearest valid chunk type. 
                                
                                b32 CanSeePlayer = ContainsChunkType(Entity, PlayerChunk->MainType);
                                
                                if(LookingForPosition(Entity, PlayerPos) && !CanSeePlayer) {
                                    EndPath(Entity);
                                }
                                
                                if(Entity->LastSearchPos != PlayerPos && CanSeePlayer && Entity->WentToNewMove) {
                                    v2 TargetPos_r32 = WorldChunkInMeters*V2i(PlayerPos);
                                    
                                    WasSuccessful = InitializeMove(GameState, Entity, TargetPos_r32);
                                    if(WasSuccessful) {
                                        Entity->LastSearchPos = PlayerPos;
                                    }
                                } 
                                //Then if we couldn't find the player we just move a random direction
                                if(!WasSuccessful && !HasMovesLeft(Entity)) {
                                    v2i GridPos = GetGridLocation(Entity->Pos);
                                    
                                    world_chunk *CheckPointParentIdInfo =  GetOrCreateWorldChunk(Entity->ParentCheckPointIds, GridPos.X, GridPos.Y, 0, ChunkNull, &GameState->ChunkFreeList);
                                    
                                    if(CheckPointParentIdInfo) {
                                        u32 ParentID = (u32)CheckPointParentIdInfo->Type;
                                        Assert(ParentID);
                                        
                                        entity *CheckPointParent= FindEntityFromID(GameState, ParentID);
                                        if(CheckPointParent && CheckPointParent->CheckPointCount) { //Parent has CheckPoints
                                            entity *CurrentCheckPoint = FindEntityFromID(GameState, CheckPointParent->CheckPointIds[Entity->CheckPointAt]);
                                            
                                            v2 TargetPos_r32 = WorldChunkInMeters*GetGridLocationR32(CurrentCheckPoint->Pos);
                                            
                                            b32 SuccessfulMove = InitializeMove(GameState, Entity, TargetPos_r32, false);
                                            Assert(SuccessfulMove);
                                            
                                            Entity->CheckPointAt++;
                                            if(Entity->CheckPointAt >= CheckPointParent->CheckPointCount) {
                                                Entity->CheckPointAt = 0;
                                            }
                                        }
                                    }
                                    
                                    
                                }
                                UpdateEntityPositionViaFunction(GameState, Entity, dt);
                                if(GetGridLocation(Entity->Pos) == GetGridLocation(Player->Pos)) {
                                    GameState->GameMode = GAMEOVER_MODE;
                                }
                            }
#if 0
                            PushRect(RenderGroup, EntityAsRect, 1, V4(1, 0, 0, 1));
#else 
                            if(!IsEmpty(&Entity->AnimationListSentintel)) {
                                
                                PushCurrentAnimationBitmap(RenderGroup, Entity, EntityRelP, BufferRect, 3.0f);
                                //// NOTE(OLIVER): Preparing to look for new animation State
                                
                                animation *NewAnimation = 0;
                                r32 DirectionValue = GetDirectionInRadians(Entity);
                                
#define USE_ANIMATION_STATES 0
#if USE_ANIMATION_STATES
                                r32 Qualities[ANIMATE_QUALITY_COUNT] = {};
                                r32 Weights[ANIMATE_QUALITY_COUNT] = {1.0f};
                                
                                Qualities[DIRECTION] = DirectionValue; 
                                
                                NewAnimation = GetAnimation(GameState->KnightAnimations, GameState->KnightAnimationCount, Qualities, Weights);
#else 
                                char *AnimationName = 0;
                                r32 Speed = Length(Entity->Velocity);
                                if(Speed < 1.0f) {
                                    AnimationName = "Man_Idle";
                                } else {
                                    if (DirectionValue == 0) {
                                        AnimationName = "Man_Run_Right";
                                    } 
                                    else if (DirectionValue == 0.25f*TAU32) {
                                        AnimationName = "Man_Run_Up";
                                    } 
                                    else if (DirectionValue == 0.5f*TAU32) {
                                        AnimationName = "Man_Run_Left";
                                    } 
                                    else if (DirectionValue == 0.75f*TAU32) {
                                        AnimationName = "Man_Run_Down";
                                    } 
                                }
                                
                                NewAnimation = FindAnimation(GameState->ManAnimations, GameState->ManAnimationCount, AnimationName);
#endif
                                
                                Assert(NewAnimation);
                                UpdateAnimation(GameState, Entity, dt, NewAnimation);
                                ///
                            }
                            //PushBitmap(RenderGroup, V3(EntityRelP, 1), &GameState->Monster,  1.0f*WorldChunkInMeters, BufferRect);
#endif
                            //PushBitmap(RenderGroup, V3(EntityRelP, 1), &GameState->MossBlockBitmap,  BufferRect);
                        } break;
                        case Entity_Block: {
                            if(!GameState->UIState->GamePaused) {
                                UpdateEntityPositionViaFunction(GameState, Entity, dt);
                            }
                            /* Draw Block as Filled Rect
                            PushRect(RenderGroup, EntityAsRect, 1, V4(1, 0.9f, 0.6f, 1)); 
                            
                            //PushRectOutline(RenderGroup, EntityAsRect, 1, V4(0, 0, 0, 1)); 
                            */
                            PushBitmap(RenderGroup, V3(EntityRelP, 1), &GameState->Crate,  0.8f*WorldChunkInMeters, BufferRect);
                            //This is for the UI level editor
                            if(!IsValidGridPosition(GameState, Entity->Pos)) {
                                PushRect(RenderGroup, EntityAsRect, 1, V4(1, 0, 0, 0.4f)); 
                            }
                            //
                            
                        } break;
                        case Entity_Dropper: {
                            UpdateEntityPositionViaFunction(GameState, Entity, dt);
                            if(Entity->WentToNewMove) {
                                entity* Changer = AddEntity(GameState, Entity->Pos, WorldChunkInMeters*V2(0.5f, 0.5f), Entity_Chunk_Changer, GameState->EntityIDAt++);
                                AddChunkTypeToChunkChanger(Changer, ChunkLight);
                                AddChunkTypeToChunkChanger(Changer, ChunkDark);
                                
                            }
                            
                            if(!HasMovesLeft(Entity)) {
                                //Add to a list to remove afterwards
                                RemoveEntity(GameState, EntityIndex);
                            } else {
                                PushRect(RenderGroup, EntityAsRect, 1, V4(0.8f, 0.8f, 0, 1));
                            }
                        } break;
                        case Entity_Chunk_Changer: {
                            
                            PushRect(RenderGroup, EntityAsRect, 1, V4(1, 0, 0, 0.1f));
                            
                            v2i GridP = GetGridLocation(Entity->Pos);
                            
                            world_chunk *Chunk = GetOrCreateWorldChunk(GameState->Chunks, GridP.X, GridP.Y, 0, ChunkNull);
                            Assert(Chunk);
                            b32 MoveToNextChunk = UpdateTimer(&Entity->ChunkTimer, dt);
                            if(MoveToNextChunk) {
                                Chunk->Type = Entity->ChunkList[Entity->ChunkAt];
                                if(Entity->LoopChunks) {
                                    Entity->ChunkAt = WrapU32(0, Entity->ChunkAt + 1, Entity->ChunkListCount - 1);
                                } else {
                                    Entity->ChunkAt = ClampU32(0, Entity->ChunkAt + 1, Entity->ChunkListCount - 1);
                                }
                            }
                            //Destory Entity if past its life span
                            Entity->LifeSpan -= dt;
                            if(Entity->LifeSpan <= 0.0f) {
                                b32 WasRemoved = RemoveEntity(GameState, EntityIndex);
                                Assert(WasRemoved);
                            }
                            
                            
                        } break;
                        case Entity_Home: {
                            PushRect(RenderGroup, EntityAsRect, 1, V4(1, 0, 1, 1));
                            entity *Philospher = GetEntity(GameState, Entity->Pos, Entity_Philosopher);
                            if(Philospher) {
                                GameState->GameMode = WIN_MODE;
                            }
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
            
            
            //////////////UI Update And Rendering////////////////
            if(GameState->ShowHUD) {
                
                ui_loop_info UIInfo = InitUILoopInfo();
                ui_element *NextHotEntity = 0;
                
                for(u32 UIIndex = 0; UIIndex < GameState->UIState->ElmCount; ++UIIndex) {
                    ui_element *Element = GameState->UIState->Elements + UIIndex;
                    
                    //loop through all ui elements
                    if(!UIInfo.IndexesProcessed[UIIndex] && Element->IsValid) { //Check if it has been drawn
                        Assert(UIInfo.LastElmCount == 0);
                        UIInfo.StartP = UIInfo.PosAt = Element->Set.Pos;
                        
                        //Assert(Element->Type == UI_Parent);
                        if(Element->Type == UI_Parent) {
                            rect2 ClipRect = Rect2MinDim(UIInfo.StartP, V2(UIInfo.MenuWidth, UIInfo.StartP.Y + MAX_S32));
                            b32 DrawBacking = true;
                            
                            /*
                            rect2 BackingRect = ClipRect;
                            BackingRect.Max.Y = 500;
                            PushRect(OrthoRenderGroup, BackingRect, 1, V4(0.2f, 0.2f, 0.2f, 0.5f));
                            */
                            PushOntoLastElms(&UIInfo, Element);
                            
                            for(;;) {
                                GetLatestElm(&UIInfo);
                                
                                for(UIInfo.ChildAt = UIInfo.ParentElm->TempChildrenCount;
                                    UIInfo.ChildAt <  UIInfo.ParentElm->ChildrenCount; 
                                    ) {
                                    
                                    BeginUIElmsLoop(&UIInfo, GameState->UIState, UI_CHILD_LOOP);
                                    
                                    ////////Drawing the ui Element//////
                                    r32 MenuHeight = 500;
                                    v2 TempPosAt = {};
                                    
                                    rect2 ElementAsRect = {};
                                    switch(UIInfo.ChildElm->Type) {
                                        case UI_Moveable:{
                                            v2 Dim = V2(UIInfo.MenuWidth, (r32)GameState->DebugFont->LineAdvance);
                                            ElementAsRect = Rect2MinDim(V2(UIInfo.PosAt.X, UIInfo.PosAt.Y), Dim);
                                            PushRectOutline(OrthoRenderGroup, ElementAsRect, 1, V4(0.5f, 1, 0, 1)); 
                                            UIInfo.PosAt.Y += GetHeight(ElementAsRect);
                                            UIInfo.ChildElm->Set.Dim = Dim;
                                            
                                            if(!UIInfo.ChildElm->Set.Active) {
                                                UIInfo.LookAtChildren = false;
                                            }
                                        } break;
                                        case UI_DropDownBoxParent: {
                                            enum_array_data *Data =(enum_array_data *)UIInfo.ChildElm->Set.ValueLinkedToPtr;
                                            
                                            char **NameArray = (char **)UIInfo.ChildElm->Set.EnumArray.Array;
                                            UIInfo.ChildElm->Set.Name = NameArray[Data->Index];
                                            if(UIInfo.ChildElm->Set.Active) {
                                                if(UIInfo.ChildElm->ChildrenCount) {
                                                    
                                                    DrawMenuItemWithText(UIInfo.ChildElm, OrthoRenderGroup, &UIInfo.PosAt, GameState->DebugFont, &ElementAsRect, ClipLeftX(UIInfo.PosAt, ClipRect), UIState->InteractingWith);
                                                    
                                                }
                                            } 
                                        } break;
                                        case UI_DropDownBox: {
                                            Assert(UIInfo.ParentElm->Type == UI_DropDownBoxParent);
                                            if(!UIInfo.ParentElm->Set.Active) { //This is the parent
                                                rect2 MenuClipRect = ClipLeftX(UIInfo.PosAt, ClipRect);
                                                MenuClipRect.Max.Y = (ClipRect.Min.Y + MenuHeight);
                                                
                                                //This needs the stencil buffer to clip the rect space. 
                                                b32 DrawMenuItem = true;
                                                b32 DisplayMenuItem = true;
                                                if(UIInfo.PosAt.Y < TempPosAt.Y) {
                                                    DisplayMenuItem = false;
                                                }
                                                if(UIInfo.PosAt.Y > MenuClipRect.Max.Y) {
                                                    DrawMenuItem = false;
                                                }
                                                //
                                                if(DrawMenuItem) {
                                                    DrawMenuItemWithText(UIInfo.ChildElm, OrthoRenderGroup, &UIInfo.PosAt, GameState->DebugFont, &ElementAsRect, MenuClipRect, UIState->InteractingWith,  DisplayMenuItem);
                                                }
                                            } 
                                            
                                        } break;
                                        case UI_CheckBox: {
                                            DrawMenuItemWithText(UIInfo.ChildElm, OrthoRenderGroup, &UIInfo.PosAt, GameState->DebugFont, &ElementAsRect, ClipLeftX(UIInfo.PosAt, ClipRect), UIState->InteractingWith);
                                        } break;
                                        case UI_TextBox: {
                                            DrawMenuItemWithText(UIInfo.ChildElm, OrthoRenderGroup, &UIInfo.PosAt, GameState->DebugFont, &ElementAsRect, ClipLeftX(UIInfo.PosAt, ClipRect), UIState->InteractingWith);
                                        } break;
                                        case UI_Slider: {
                                            if(!UIInfo.ParentElm->Set.Active) {
                                                rect2 OutlineDim = Rect2MinDim(UIInfo.PosAt - V2(UIInfo.IndentSpace, 0), V2(UIInfo.IndentSpace, MenuHeight));
                                                PushRectOutline(OrthoRenderGroup, OutlineDim, 1, V4(0, 0, 0, 1));
                                                v2 ScrollPos = {};
                                                r32 ScrollAt01 = *((r32 *)UIInfo.ChildElm->Set.ValueLinkedToPtr);
                                                ScrollPos.Y = ScrollAt01;
                                                ScrollPos.Y = Lerp(UIInfo.ChildElm->Set.Min, ScrollPos.Y, UIInfo.ChildElm->Set.Max);
                                                ScrollPos.X = UIInfo.PosAt.X - UIInfo.IndentSpace;
                                                
                                                rect2 HandleDim = Rect2CenterDim(ScrollPos, V2(UIInfo.IndentSpace, UIInfo.IndentSpace));
                                                HandleDim.Min.X += 0.5f*UIInfo.IndentSpace;
                                                HandleDim.Max.X += 0.5f*UIInfo.IndentSpace;
                                                v4 Color = V4(1, 1, 1, 1);
                                                PushRect(OrthoRenderGroup, HandleDim, 1, Color);
                                                ElementAsRect = HandleDim;
                                                UIInfo.ChildElm->Set.Min = OutlineDim.Min.Y;
                                                UIInfo.ChildElm->Set.Max = OutlineDim.Max.Y;
                                                
                                                TempPosAt = UIInfo.PosAt;
                                                UIInfo.PosAt.Y -= MenuHeight*ScrollAt01;
                                            }
                                            
                                        } break;
                                        case UI_Button: {
                                            DrawMenuItemWithText(UIInfo.ChildElm, OrthoRenderGroup, &UIInfo.PosAt, GameState->DebugFont, &ElementAsRect, ClipLeftX(UIInfo.PosAt, ClipRect), UIState->InteractingWith);
                                        } break;
                                        case UI_Entity: {
                                            //Already Rendered Above
                                            u32 ID = CastVoidAs(u32, UIInfo.ChildElm->Set.ValueLinkedToPtr);
                                            entity *Entity = FindEntityFromID(GameState, ID);
                                            v2 EntityRelP = Entity->Pos - CamPos;
                                            
                                            ElementAsRect = Rect2CenterDim(EntityRelP, Entity->Dim);
                                            if(InBounds(ElementAsRect, MouseP_PerspectiveSpace)) {
                                                NextHotEntity = UIInfo.ChildElm;
                                                //AddToOutBuffer("Is Hot!\n");
                                            }
                                            DrawBacking = false;
                                            
                                        } break;
                                    }
                                    
                                    
                                    UIInfo.ChildElm->Set.Pos = ElementAsRect.Min;
                                    UIInfo.ChildElm->Set.Dim = ElementAsRect.Max - ElementAsRect.Min;
                                    
                                    if(InBounds(ElementAsRect, MouseP_OrthoSpace)) {
                                        NextHotEntity = UIInfo.ChildElm;
                                    }
                                    ///////Finished Drawing UI ELEMENT ///////
                                    EndUIElmsLoop(&UIInfo, UI_CHILD_LOOP);
                                    
                                }
                                if(EndUIElmsLoop(&UIInfo, UI_OUTER_LOOP)) {
                                    break;
                                }
                            }
                            
                            if(DrawBacking) {
                                rect2 BackingRect = ClipRect;
                                BackingRect.Max.Y = UIInfo.PosAt.Y;
                                PushRect(OrthoRenderGroup, BackingRect, 1, V4(0.2f, 0.2f, 0.2f, 0.5f));
                            }
                        }
                        
                    }
                }
                
                //PushRectOutline(OrthoRenderGroup, Rect2MinMax(V2(0, 0),V2(100, 100)), 1, V4(0, 0, 0, 1)); 
                
                //AddToOutBuffer("{%f, %f}", MouseP_OrthoSpace.X, MouseP_OrthoSpace.Y);
                
                ///////////////
                
                if(IsDown(Memory->GameButtons[Button_Control]) && WasPressed(Memory->GameButtons[Button_LetterS])) {
                    AddToOutBuffer("Save Leveled!\n");
                    SaveLevelToDisk(Memory, GameState, "level1.omm");
                }
                
                editor_mode EdMode = NULL_MODE;
                
                if(!UIState->InteractingWith) {
                    if(WasPressed(Memory->GameButtons[Button_One])) {
                        EdMode = editor_mode_Values[1];
                    }
                    if(WasPressed(Memory->GameButtons[Button_Two])) {
                        EdMode = editor_mode_Values[2];
                    }
                    if(WasPressed(Memory->GameButtons[Button_Three])) {
                        EdMode = editor_mode_Values[3];
                    }
                    if(WasPressed(Memory->GameButtons[Button_Four])) {
                        EdMode = editor_mode_Values[4];
                    }
                }
                
                if(EdMode) {
                    //This clears the editor mode back to default so 
                    //we can interact with the ui panels.
                    //Another way to handle modes is to check if the
                    // the mouse is hovering over any panels. This would
                    //then override any mouse clicks to reset the
                    //mode... Oliver 4/11/17
                    SetEditorMode(GameState->UIState, EdMode);
                }
                
                
                if(WasPressed(Memory->GameButtons[Button_LeftMouse])) {
                    
                    
                    enum_array_data *ModeInfo = &UIState->EditorMode;
                    CastValue(ModeInfo, editor_mode, EditorMode);
                    v2i Cell = GetGridLocation(MouseP_PerspectiveSpace + Camera->Pos);
                    world_chunk *Chunk = GetOrCreateWorldChunk(GameState->Chunks, Cell.X, Cell.Y, 0, ChunkNull);
                    switch(EditorMode) {
                        case CREATE_MODE : {
                            
                            
                            /*
                            if(UIState->CurrentCheckPointParent) {
                                check_point_parent *Parent = UIState->CurrentCheckPointParent;
                                Assert(Parent->Count < ArrayCount(Parent->CheckPoints));
                                check_point *Point = Parent->CheckPoints + Parent->Count++;
                                Point->Pos = Cell;
                            } else */
                            {
                                enum_array_data *Info = &UIState->InitInfo;
                                char *A = Info->Type;
                                if(A) {
                                    if(Chunk) {
                                        if(DoStringsMatch(A, "entity_type")) {
                                            CastValue(Info, entity_type, Value);
                                            v2 Pos = WorldChunkInMeters*V2i(Cell);
                                            entity *Ent = GetEntityAnyType(GameState, Pos);
                                            if(Ent && IsSolid(Ent) && IsSolidDefault(Value)) {
                                                //We are Removing the entity
                                                if(Ent->Type != Entity_Camera && Ent->Type != Entity_Player) {
                                                    if(IsSolid(Ent)) {
                                                        Chunk->Type = Chunk->MainType;
                                                    } else if(Ent->Type == Entity_CheckPoint) {
                                                        entity *Par = FindEntityFromID(GameState, Ent->CheckPointParentID);
                                                        
                                                        b32 Found = false;
                                                        for(u32 i = 0; i < Par->CheckPointCount; i++) {
                                                            u32 ID = Par->CheckPointIds[i];
                                                            if(ID == Ent->ID) {
                                                                Found = true;
                                                                Par->CheckPointIds[i] = Par->CheckPointIds[--Par->CheckPointCount];
                                                            }
                                                        }
                                                        Assert(Found);
                                                    }
                                                    RemoveEntity(GameState, Ent->Index);
                                                } else {
                                                    AddToOutBuffer("There is an entity here\n");
                                                }
                                            } else {
                                                if(Value == Entity_Chunk_Changer) {
                                                    entity *Entity = AddEntity(GameState, Pos, V2(0.4f, 0.4f), Entity_Chunk_Changer, GameState->EntityIDAt++);
                                                    Entity->LoopChunks = true;
                                                    
                                                    AddChunkTypeToChunkChanger(Entity, ChunkLight);
                                                    AddChunkTypeToChunkChanger(Entity, ChunkDark);
                                                    //This should all be done via the GUI system. After you create the
                                                    //entity maybe you can then select them and see the attributes and 
                                                    //change them as neccessary! Oliver 8/10/17
                                                    
                                                } else if(Value == Entity_CheckPoint) {
                                                    u32 ParentID = UIState->CheckPointParentID;
                                                    if(ParentID) {
                                                        entity *Parent = FindEntityFromID(GameState, ParentID);
                                                        entity *Child = AddEntity(GameState, Pos, V2(0.4f, 0.4f), Value, GameState->EntityIDAt++);
                                                        Child->CheckPointParentID = ParentID;
                                                        AddCheckPointIDToCheckPointParent(Parent, Child->ID);
                                                        
                                                    } else {
                                                        AddToOutBuffer("No active CheckPoint parent\n");
                                                    }
                                                    
                                                } else if(Value == Entity_CheckPointParent) {
                                                    
                                                    entity *Entity = AddEntity(GameState, Pos, V2(0.4f, 0.4f), Value,  GameState->EntityIDAt++);
                                                    
                                                    //For Editor level purposes
                                                    UIState->CheckPointParentID = Entity->ID;
                                                    //
                                                } else {
                                                    AddToOutBuffer("Entity Added\n");
                                                    AddEntity(GameState, Pos, V2(1, 1), Value, GameState->EntityIDAt++);
                                                }
                                            }
                                        } 
                                    } else {
                                        AddToOutBuffer("No Chunk Here\n");
                                    }
                                    
                                    if(DoStringsMatch(A, "chunk_type")) {
                                        CastValue(Info, chunk_type, Value);
                                        
                                        if(Chunk && Chunk->Type == Value) {
                                            RemoveWorldChunk(GameState->Chunks, Cell.X, Cell.Y, &GameState->ChunkFreeList);
                                        } else {
                                            chunk_type PrevType = Value;
                                            if(!Chunk) {
                                                Assert(Value != ChunkNull);
                                                
                                                
                                                
                                                Chunk = GetOrCreateWorldChunk(GameState->Chunks, Cell.X, Cell.Y, &GameState->MemoryArena, Value, &GameState->ChunkFreeList);
                                            } else {
                                                PrevType = Chunk->Type;
                                            } 
                                            
                                            Chunk->Type = PrevType;
                                        }
                                    }
                                } else {
                                    AddToOutBuffer("No Entity type selected\n");
                                }
                            }
                        } break; 
                        case EDIT_CHECKPOINT_CHUNKS_MODE: {
                            if(UIState->PhilosopherID && UIState->CheckPointParentID) {
                                entity *Ent = FindEntityFromID(GameState, UIState->PhilosopherID);
                                if(Ent) {
                                    if(Chunk) {
                                        AddToOutBuffer("Check Point Parent Added\n");
                                        world_chunk *EntityChunk = GetOrCreateWorldChunk(Ent->ParentCheckPointIds, Cell.X, Cell.Y, &GameState->MemoryArena, (chunk_type)UIState->CheckPointParentID, &GameState->ChunkFreeList);
                                    }
                                }
                            }
                        } break;
                        default: { //Picking up entities with mouse. 
                            
                            if(!UIState->InteractingWith) {
                                //Here we set what we are interacting with
                                if(NextHotEntity) {
                                    Assert(NextHotEntity->IsValid);
                                    UIState->HotEntity = NextHotEntity;
                                    UIState->InteractingWith = UIState->HotEntity;
                                    
                                    ui_element *InteractEnt = UIState->InteractingWith;
                                    
                                    switch (UIState->InteractingWith->Type) {
                                        case UI_Entity: {
                                            GetEntityFromElement(UIState);
                                            Entity->RollBackPos = Entity->Pos;
                                            
                                            
                                            if(EditorMode == INTROSPECT_MODE) {
                                                
                                                ClearIntrospectWindow(UIState);
                                                
                                                if(!UIState->TempUIStack) {
                                                    
                                                    UIState->EntIntrospecting = Entity;
                                                    ui_element_settings UISet = {};
                                                    UISet.Pos = V2(300, 20);
                                                    u32 ParentElmIndex = PushUIElement(GameState->UIState, UI_Moveable,  UISet);
                                                    
                                                    ui_element *ParentElm = UIState->Elements + ParentElmIndex;
                                                    
                                                    UISet.ValueLinkedToPtr = &ParentElm->Set.ScrollAt;
                                                    AddUIElement(GameState->UIState, UI_Slider,  UISet);
                                                    {
                                                        for(u32 i = 0; i < ArrayCount(entity_Names) - 1; i += 1) {
                                                            
                                                            introspect_info *Info = entity_Names + i;
                                                            
                                                            u32 CharCount = 256;
                                                            char *Text = PushArray(&GameState->StringArena, char, CharCount);
                                                            char *At = Text;
                                                            s32 SizeOfString = PrintS(Text, CharCount, "%s %s: ", Info->Type, Info->Name);
                                                            At += SizeOfString;
                                                            
                                                            intptr Offset = Info->Offset;
                                                            
                                                            UISet.Type = Info->Type;
                                                            UISet.ValueLinkedToPtr = ((u8 *)Entity) + Offset;
                                                            UISet.Name = Text;
                                                            
                                                            memory_arena *TempArena = &UIState->TransientTextBoxArena;
                                                            InitCharBuffer(&UISet.Buffer, 256, TempArena, TempArena);
                                                            
                                                            AddUIElement(GameState->UIState, UI_TextBox, UISet);
                                                        }
                                                    }
                                                    PopUIElement(GameState->UIState);
                                                    
                                                    //This is to remove the stack later;
                                                    Assert(ParentElm->Parent);
                                                    UIState->TempUIStack = ParentElm->Parent;
                                                } else {
                                                    InvalidCodePath;
                                                }
                                            }
                                        } break;
                                        case UI_Moveable: {
                                            UIState->MouseGrabOffset = MouseP_OrthoSpace - InteractEnt->Parent->Set.Pos;
                                        } break;
                                        default: {
                                            AddToOutBuffer("There is no initial interaction for the object type \"%s\"\n", element_ui_type_Names[InteractEnt->Type]);
                                        }
                                    }
                                }
                            }
                        } break;
                    }
                }
                if(WasReleased(Memory->GameButtons[Button_LeftMouse])) {
                    
                    //MOUSE BUTTON RELEASED ACTIONS
                    if(UIState->InteractingWith) {
                        ui_element *InteractEnt = UIState->InteractingWith;
                        b32 ClearInteratingWith = true;
                        
                        switch(InteractEnt->Type) {
                            case UI_DropDownBox: {
                                ui_element *Parent = InteractEnt->Parent;
                                
                                //VIEW
                                //Parent->Set.Name = InteractEnt->Set.Name;
                                
                                //DATA
                                enum_array_data *ValueToMod = (enum_array_data *)InteractEnt->Set.ValueLinkedToPtr;
                                
                                *ValueToMod = InteractEnt->Set.EnumArray;
                                
                                //Show Active
                                Parent->Set.Active = !Parent->Set.Active;
                            } break;
                            case UI_Moveable: {
                                InteractEnt->Set.Active = !InteractEnt->Set.Active;
                            } break;
                            case UI_Button: {
                                // TODO(Oliver): Can we make this more generic?
                                char *ButtonIdentifier = InteractEnt->Set.Type;
                                if(DoStringsMatch(ButtonIdentifier, "Save Level")) {
                                    SaveLevelToDisk(Memory, GameState, "level1.omm");
                                }
                                if(DoStringsMatch(ButtonIdentifier, "Clear Introspect Window")) {
                                    ClearIntrospectWindow(UIState);
                                }
                                
                            } break;
                            case UI_Entity: {
                                GetEntityFromElement(UIState);
                                if(IsValidGridPosition(GameState, Entity->Pos)) {
                                    //Success! Do nothing.
                                } else {
                                    Entity->Pos
                                        = Entity->RollBackPos;
                                }
                                
                            } break;
                            case UI_CheckBox: {
                                b32 Value = *((b32 *)InteractEnt->Set.ValueLinkedToPtr);
                                *(b32 *)InteractEnt->Set.ValueLinkedToPtr = !Value;
                            } break;
                            case UI_TextBox: {
                                ClearInteratingWith = false; 
                            } break;
                            case UI_DropDownBoxParent: {
                                InteractEnt->Set.Active = !InteractEnt->Set.Active;
                            } break;
                            default: {
                                AddToOutBuffer("There is no release interaction for the object type \"%s\"\n", element_ui_type_Names[InteractEnt->Type]);
                            }
                        }
                        
                        if(ClearInteratingWith) {
                            UIState->InteractingWith = 0;
                        }
                    }
                }
                
                if(UIState->InteractingWith) {
                    //MOUSE BUTTON DOWN ACTIONS
                    
                    ui_element *Interact = UIState->InteractingWith;
                    switch(UIState->InteractingWith->Type) {
                        case UI_Entity: {
                            GetEntityFromElement(UIState);
                            
                            v2 NewEntityPos = WorldChunkInMeters*GetGridLocationR32(MouseP_PerspectiveSpace + CamPos);
                            
                            SetChunkType(GameState, Entity->Pos, ChunkMain);
                            Entity->Pos = NewEntityPos;
                            
                            if(IsSolid(Entity)) {
                                SetChunkType(GameState, Entity->Pos, ChunkEntity);
                            }
                        } break;
                        case UI_Moveable:{
                            rect2 BarDim = Rect2MinDim(V2(0, 0), Interact->Set.Dim);
                            v2 ClampedMouseP = Clamp(MouseP_OrthoSpace - UIState->MouseGrabOffset, Subtract(BufferRect, BarDim));
                            //We assume parent will always be the most outer ui element...Have an Assert here. 
                            Interact->Parent->Set.Pos = ClampedMouseP;
                        } break;
                        case UI_Slider:{
                            r32 tValue = InverseLerp(Interact->Set.Min, MouseP_OrthoSpace.Y, Interact->Set.Max);
                            
                            tValue = Clamp(0, tValue, 1);
                            *(r32 *)Interact->Set.ValueLinkedToPtr = tValue;
                        } break;
                        default: {
                            AddToOutBuffer("There is no down interaction for the object type \"%s\"\n", element_ui_type_Names[Interact->Type]); //TODO(oliver): Maybe GetEnumAlias(entity_type, TypeValue); ?
                        }
                    }
                }
                
                ////////////Hover Indication /////////////////////
                ui_element *Elm = GameState->UIState->InteractingWith;
                render_group *ThisRenderGroup = OrthoRenderGroup;
                if(Elm) {
                    Assert(Elm->IsValid);
                    rect2 ElmAsRect = Rect2MinDim(Elm->Set.Pos, Elm->Set.Dim);
                    if(Elm->Type == UI_Entity) {
                        ThisRenderGroup = RenderGroup;
                        GetEntityFromElement_(Elm);
                        v2 ElmRelP = Entity->Pos - CamPos;
                        ElmAsRect = Rect2CenterDim(ElmRelP, Entity->Dim);
                    }
                    PushRectOutline(ThisRenderGroup, ElmAsRect, 1, V4(1.0f, 0, 0, 1)); 
                } else if(NextHotEntity){
                    Elm = NextHotEntity;
                    if(Elm->IsValid) {
                        rect2 ElmAsRect = Rect2MinDim(Elm->Set.Pos, Elm->Set.Dim);
                        if(Elm->Type == UI_Entity) {
                            ThisRenderGroup = RenderGroup;
                            GetEntityFromElement_(Elm);
                            v2 ElmRelP = Entity->Pos - CamPos;
                            ElmAsRect = Rect2CenterDim(ElmRelP, Entity->Dim);
                        } 
                        
                        PushRectOutline(ThisRenderGroup, ElmAsRect, 1, V4(0.2f, 0, 1, 1)); 
                    }
                } else {
                    v2 Cell = GetGridLocationR32(MouseP_PerspectiveSpace + Camera->Pos);
                    
                    v2 ElmRelP = WorldChunkInMeters*Cell - CamPos;
                    
                    rect2 ElmAsRect = Rect2CenterDim(ElmRelP, V2(WorldChunkInMeters, WorldChunkInMeters));
                    
                    PushRectOutline(RenderGroup, ElmAsRect, 1, V4(0.5f, 1, 0, 1)); 
                }
                
            }
            if(GameState->PlayerIsSettingPath) {
                if(GameState->PathToSet.Count > 0) {
#define RENDER_CONTROL_PATH_ORTHO 0
#if RENDER_CONTROL_PATH_ORTHO
                    v2 LastPos = GameState->PathToSet.Points[0];
                    v2 Pos = 0.5f*OrthoRenderGroup->ScreenDim;
                    v2 Dim = V2(50, 50);
                    for(u32 i = 0; i < GameState->PathToSet.Count; ++i) {
                        v2 ThisPos = GameState->PathToSet.Points[i];
                        v2 Shift = ThisPos - LastPos;
                        Pos += Hadamard(Shift, Dim);
                        rect2 ElmAsRect = Rect2MinDim(Pos, Dim);
                        PushRectOutline(OrthoRenderGroup, ElmAsRect, 1, V4(0.2f, 0, 1, 1)); 
                        LastPos = ThisPos;
                        
                    }
#else
                    v2 Dim = WorldChunkInMeters*V2(1, 1);
                    for(u32 i = 0; i < GameState->PathToSet.Count; ++i) {
                        
                        entity TempEntity = {};
                        TempEntity.Dim = Dim;
                        TempEntity.Pos = GameState->PathToSet.Points[i];
                        rect2 ElmAsRect = GetEntityInCameraSpace(&TempEntity, CamPos);
                        PushRectOutline(RenderGroup, ElmAsRect, 1, V4(0.2f, 0, 1, 1)); 
                        
                        
                    }
#endif
                }
            }
            
            if(GameState->RenderConsole) {
                RenderConsole(&DebugConsole, dt); 
            }
            
            //PushBitmap(OrthoRenderGroup, V3(MouseP_OrthoSpace, 1), &GameState->MagicianHandBitmap, 100.0f, BufferRect);
            rect2 MouseRect = Rect2CenterDim(MouseP_OrthoSpace, V2(20, 20));
            PushRectOutline(OrthoRenderGroup, MouseRect, 1, V4(0.2f, 0, 1, 1)); 
            ///////////////////
            
        } //END OF PLAY_MODE 
    } //switch on GAME_MODE
    
    
    //RenderGroupToOutput(RenderGroup);
    //RenderGroupToOutput(&OrthoRenderGroup);
    
    ReleaseMemory(&PerFrameMemory);
    
}

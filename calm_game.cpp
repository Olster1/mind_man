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
#include "calm_console.cpp"
#include "calm_ui.cpp"
#include "calm_entity.cpp"
#include "calm_animation.cpp"
#include "calm_meta.h"
#include "meta_enum_arrays.h"

#define GetChunkHash(X, Y) (Abs(X*19 + Y*23) % WORLD_CHUNK_HASH_SIZE)
internal world_chunk *GetOrCreateWorldChunk(world_chunk **Chunks, s32 X, s32 Y, memory_arena *Arena, chunk_type Type, world_chunk **FreeListPtr = 0) {
    
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
        Assert(Type != ChunkNull);
        if(FreeListPtr && *FreeListPtr) {
            Result = *FreeListPtr;
            *FreeListPtr = (*FreeListPtr)->Next;
        } else {
            Result = PushStruct(Arena, world_chunk);
        }
        Result->X = X;
        Result->Y = Y;
        Result->MainType = Result->Type = Type;
        Result->Next = Chunks[HashIndex];
        Chunks[HashIndex] = Result;
    }
    
    return Result;
}

internal b32
RemoveWorldChunk(world_chunk **Chunks, s32 X, s32 Y, world_chunk **FreeListPtr) {
    
    b32 Removed = false;
    u32 HashIndex = GetChunkHash(X, Y);
    world_chunk **ChunkPtr = Chunks + HashIndex;
    
    while(*ChunkPtr) {
        world_chunk *Chunk = *ChunkPtr;
        if(Chunk->X == X && Chunk->Y == Y) {
            (*ChunkPtr) = Chunk->Next;
            Chunk->Next = *FreeListPtr;
            *FreeListPtr = Chunk;
            Removed = true;
            break;
        }
        
        ChunkPtr = &Chunk->Next;
    } 
    return Removed;
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
    Info->VisitedHash = PushArray(TempArena, world_chunk *, ArrayCount(GameState->Chunks), true);
    
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

inline v2i GetGridLocation(v2 Pos) {
    r32 MetersToWorldChunks = 1.0f / WorldChunkInMeters;
    
    v2i Result = ToV2i_floor(MetersToWorldChunks*Pos); 
    
    return Result;
}

inline v2i GetClosestGridLocation(v2 Pos) {
    r32 MetersToWorldChunks = 1.0f / WorldChunkInMeters;
    
    v2i Result = ToV2i_ceil(MetersToWorldChunks*Pos); 
    
    return Result;
}

inline v2 GetClosestGridLocationR32(v2 Pos) {
    v2i TempLocation = GetClosestGridLocation(Pos);
    v2 Result = V2i(TempLocation.X, TempLocation.Y);
    return Result;
}

inline v2 GetGridLocationR32(v2 Pos) {
    v2i TempLocation = GetGridLocation(Pos);
    v2 Result = V2i(TempLocation.X, TempLocation.Y);
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

internal b32 UpdateEntityPositionViaFunction(game_state *GameState, entity *Entity, r32 dt) {
    b32 WentToNewMove = false;
    Entity->IsAtEndOfMove = true;
    if(HasMovesLeft(Entity)) {
        Entity->IsAtEndOfMove = false;
        Assert(Entity->VectorIndexAt > 0);
        v2i GridA = ToV2i(Entity->Path.Points[Entity->VectorIndexAt - 1]);
        v2 A = WorldChunkInMeters*V2i(GridA);
        
        v2i GridB = ToV2i(Entity->Path.Points[Entity->VectorIndexAt]);
        v2 B = WorldChunkInMeters*V2i(GridB);
        
        if(!IsValidChunkType(GameState, Entity, GridB)) {
            chunk_type Types[] = {ChunkLight, ChunkDark};
            v2i NewPos = SearchForClosestValidChunkType(GameState, GridA, Types,ArrayCount(Types));
            Entity->Pos = WorldChunkInMeters*V2i(NewPos);
            
        }
        
        Entity->MoveT += dt;
        
        r32 tValue = GetFractionOfMove(Entity);
        
        Entity->Pos = SineousLerp0To1(A, tValue, B);
        
        //For Animation
        v2 Direction = Normal(B - A);
        Entity->Velocity = 20.0f*Direction;
        //
        
        if(Entity->MoveT / Entity->MovePeriod >= 1.0f) {
            Entity->Pos = B;
            Entity->MoveT -= Entity->MovePeriod;
            Entity->VectorIndexAt++;
            WentToNewMove = true;
            Entity->IsAtEndOfMove = false;
#if 1
            if(HasMovesLeft(Entity)) {
                
                v2i GoingTo = ToV2i(Entity->Path.Points[Entity->VectorIndexAt]);
                if(!IsValidChunkType(GameState, Entity, GoingTo)) {
                    Entity->Path.Count = 0;
                    Assert(!HasMovesLeft(Entity));
                }
            }
#endif
        }
    } else {
        Entity->Velocity = {};
    }
    
    return WentToNewMove;
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

inline entity *
AddEntity(game_state *GameState, v2 Pos, entity_type EntityType) {
    world_chunk *Chunk = GetOrCreateWorldChunk(GameState->Chunks, (s32)Pos.X, (s32)Pos.Y, 0, ChunkNull);
    if(Chunk) { 
        if(EntityType == Entity_Block) { Chunk->Type = ChunkBlock; }
    } else {
        chunk_type ChunkType = (EntityType == Entity_Block) ? ChunkBlock: ChunkLight;
        Chunk = GetOrCreateWorldChunk(GameState->Chunks, (s32)Pos.X, (s32)Pos.Y, &GameState->MemoryArena, ChunkType, &GameState->ChunkFreeList);
        Chunk->MainType = ChunkLight;
    }
    
    entity *Block = InitEntity(GameState, Pos, V2(1, 1), EntityType, GameState->EntityIDAt++);
    AddChunkType(Block, ChunkLight);
    if(EntityType != Entity_Philosopher) {
        AddChunkType(Block, ChunkDark);
    }
    if(EntityType == Entity_Block) {
        AddChunkType(Block, ChunkBlock);
    }
    
    return Block;
}

#define GetEntity(State, Pos, Type, ...) GetEntity_(State, Pos, Type, __VA_ARGS__)
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
    
    rect2 EntityAsRect = Rect2MinDim(EntityRelP, Entity->Dim);
    return EntityAsRect;
}

internal b32 
InitializeMove(game_state *GameState, entity *Entity, v2 TargetP_r32, b32 StrictMove = false) {
    
    r32 MetersToWorldChunks = 1.0f / WorldChunkInMeters;
    
    v2i TargetP = ToV2i_floor(MetersToWorldChunks*TargetP_r32); 
    v2i StartP = GetGridLocation(Entity->Pos);
    
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
    
    return FoundTargetP;
}

internal b32 SaveLevelToDisk(game_memory *Memory, game_state *GameState, char *FileName) {
    
    game_file_handle Handle = Memory->PlatformBeginFileWrite(FileName);
    b32 WasSuccessful = false;
    temp_memory TempMem = MarkMemory(&GameState->ScratchPad);
    if(!Handle.HasErrors) {
        
        u32 TotalSize = KiloBytes(1);
        char *MemoryToWrite = (char *)PushSize(&GameState->ScratchPad, TotalSize);
        char *MemoryAt = MemoryToWrite;
        
#define GetSize() GetRemainingSize(MemoryAt, MemoryToWrite, TotalSize)
        
        MemoryAt += PrintS(MemoryAt, GetSize(), "//CHUNKS X Y Type MainType\n");
        
        for(u32 i = 0; i < ArrayCount(GameState->Chunks); ++i) {
            world_chunk *Chunk = GameState->Chunks[i];
            while(Chunk) {
                MemoryAt +=  PrintS(MemoryAt, GetSize(), "%d %d %d %d\n", Chunk->X, Chunk->Y, Chunk->Type, Chunk->MainType);
                
                Chunk = Chunk->Next;
            }
        }
        
        
        MemoryAt +=  PrintS(MemoryAt, GetSize(), "//ENTITIES Pos Dim Type ID\n");
        
        for(u32 i = 0; i < GameState->EntityCount; ++i) {
            entity *Entity = GameState->Entities + i;
            
            MemoryAt +=  PrintS(MemoryAt, GetSize(), "%f %f %f %f %d %d ", Entity->Pos.X, Entity->Pos.Y, Entity->Dim.X, Entity->Dim.Y, Entity->Type, Entity->ID);
            
            //Add chunk types
            forN_(Entity->ChunkTypeCount, TypeIndex) {
                MemoryAt +=  PrintS(MemoryAt, GetSize(), "%d ", (u32)Entity->ValidChunkTypes[TypeIndex]);
            }
            
            MemoryAt +=  PrintS(MemoryAt, GetSize(), "\n");
            
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
                    DATA_ENTITIES,
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
                                    char *ChunkID = "CHUNKS";
                                    char *EntitiesID = "ENTITIES";
                                    
                                    At++;
                                    
                                    if(DoStringsMatch(ChunkID, At, StringLength(ChunkID))) {
                                        DataType = DATA_CHUNKS;
                                    } else if(DoStringsMatch(EntitiesID, At, StringLength(EntitiesID))) {
                                        DataType = DATA_ENTITIES;
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
                                    value_data *Data = Datas + DataAt++;
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
                                    
                                } else {
                                    InvalidCodePath;
                                }
                            }
                            
                        }
                        
                    }
                    
                    Assert(DataType != DATA_NULL);
                    
                    if(DataType == DATA_CHUNKS) {
                        Assert(DataAt == 4);
                        world_chunk *Chunk = GetOrCreateWorldChunk(GameState->Chunks, CastAs(s32, Datas[0].Value), CastAs(s32, Datas[1].Value), &GameState->MemoryArena, (chunk_type)CastAs(s32, Datas[3].Value), &GameState->ChunkFreeList);
                        Chunk->Type = (chunk_type)CastAs(s32, Datas[2].Value);
                        
                    } else if(DataType == DATA_ENTITIES) {
                        u32 EntityInfoCount = 6;
                        Assert(DataAt >= EntityInfoCount);
                        v2 Pos = V2(CastAs(r32, Datas[0].Value), CastAs(r32, Datas[1].Value));
                        v2 Dim = V2(CastAs(r32, Datas[2].Value), CastAs(r32, Datas[3].Value));
                        entity_type Type = (entity_type)CastAs(s32, Datas[4].Value);
                        u32 ID = CastAs(u32, Datas[5].Value);
                        
                        if(GameState->EntityIDAt <= ID) {
                            GameState->EntityIDAt = ID + 1;
                        }
                        
                        if(Type == Entity_Player) {
                            GameState->RestartPlayerPosition = Pos;
                        }
                        
                        entity *Entity = InitEntity(GameState, Pos, Dim, Type, ID);
                        for(u32 ValueIndex = EntityInfoCount; ValueIndex < DataAt; ++ValueIndex) {AddChunkType(Entity, (chunk_type)CastAs(s32, Datas[ValueIndex].Value));
                        }
                    }
                }
            }
        }
        ReleaseMemory(&TempMem);
        
    } else {
        
        entity *Player = InitEntity(GameState, V2(-1, -1), V2(WorldChunkInMeters, WorldChunkInMeters), Entity_Player, GameState->EntityIDAt++);
        
        GameState->RestartPlayerPosition = V2(-1, -1);
        
        AddChunkType(Player, ChunkDark);
        AddChunkType(Player, ChunkLight);
        
        entity *Camera = InitEntity(GameState, V2(0, 0), V2(0, 0), Entity_Camera, GameState->EntityIDAt++);
        Camera->Collides = false;
        
        //AddWorldChunks(GameState, 200, 0, 100, ChunkLight);
        //AddWorldChunks(GameState, 200, -100, 0, ChunkDark);
        
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
#if CREATE_PHILOSOPHER
        entity *Philosopher = InitEntity(GameState, V2(2, 2), V2(1, 1), Entity_Philosopher, GameState->EntityIDAt++);
        Philosopher->MovePeriod = 0.4f;
        AddChunkType(Philosopher, ChunkLight);
        
        v2i PhilosopherPos = GetGridLocation(Philosopher->Pos);
        GetOrCreateWorldChunk(GameState->Chunks, PhilosopherPos.X, PhilosopherPos.Y, &GameState->MemoryArena, ChunkLight);
#endif
        
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
        InitializeMemoryArena(&GameState->MemoryArena, (u8 *)Memory->GameStorage + sizeof(game_state), Memory->GameStorageSize - sizeof(game_state));
        
        GameState->PerFrameArena = SubMemoryArena(&GameState->MemoryArena, MegaBytes(2));
        
        GameState->ScratchPad = SubMemoryArena(&GameState->MemoryArena, MegaBytes(2));
        
        GameState->RenderArena = SubMemoryArena(&GameState->MemoryArena, MegaBytes(2));
        
        GameState->StringArena = SubMemoryArena(&GameState->MemoryArena, KiloBytes(1));
        
        for(u32 i = 0; i < ArrayCount(GameState->ThreadArenas); ++i) {
            GameState->ThreadArenas[i] = SubMemoryArena(&GameState->MemoryArena, KiloBytes(1));
        }
        
        GameState->UIState = PushStruct(&GameState->MemoryArena, ui_state);
        
        /////////////////////////////////
        
        GameState->RenderConsole = true;
        
#if 0
        //"Moonlight_Hall.wav","Faro.wav"
        GameState->BackgroundMusic = LoadWavFileDEBUG(Memory, "podcast1.wav", &GameState->MemoryArena);
        //PushSound(GameState, &GameState->BackgroundMusic, 1.0, false);
        GameState->FootstepsSound[0] = LoadWavFileDEBUG(Memory, "foot_steps_sand1.wav", &GameState->MemoryArena);
        
        GameState->FootstepsSound[1] = LoadWavFileDEBUG(Memory, "foot_steps_sand2.wav", &GameState->MemoryArena);
        
        
        GameState->MossBlockBitmap = LoadBitmap(Memory, 0, "moss_block1.bmp");
#endif
        GameState->MagicianHandBitmap = LoadBitmap(Memory, 0, "magician_hand.bmp");
        
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
        
        //GameState->Crate = LoadImage(Memory, 0, "crate.bmp ");
        GameState->Crate = LoadBitmap(Memory, 0, "door4.bmp", V2(0.5f, 0.3f));
        GameState->Water = LoadBitmap(Memory, 0, "water3.bmp");
        GameState->Desert = LoadBitmap(Memory, 0, "desert.bmp");
        
        GameState->Door = LoadBitmap(Memory, 0, "door4.bmp", V2(0.5f, 0.05f));
        
        GameState->Monster = LoadBitmap(Memory, 0, "monster1.bmp");
        
        GameState->BackgroundImage = LoadImage(Memory, &GameState->MemoryArena, "background_sun.png", V2(0.5f, 0.5f));
        
        
        GameState->KnightAnimationCount = 0;
        
        char *BaseName = "knight/knight iso char_";
        //char *BaseName = "knight iso char_";
        
#define CREATE_NAME(Append) CREATE_NAME_(Append##.png)
#define CREATE_NAME_(Append) CREATE_NAME__(#Append)
#define CREATE_NAME__(Append) Concat(&GameState->StringArena, BaseName, Append)
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
        UISet.Pos = V2(0.7f*(r32)Buffer->Width, (r32)Buffer->Height);
        UISet.Name = 0;
        
        PushUIElement(GameState->UIState, UI_Moveable,  UISet);
        {
            UISet.Type = UISet.Name = "Save Level";
            AddUIElement(GameState->UIState, UI_Button, UISet);
            
            UISet.ValueLinkedToPtr = &GameState->RenderMainChunkType;
            UISet.Name = "Render Main Chunk Type";
            AddUIElement(GameState->UIState, UI_CheckBox, UISet);
        }
        PopUIElement(GameState->UIState);
        
        
        UISet= {};
        UISet.Pos = V2(0, (r32)Buffer->Height);
        
        PushUIElement(GameState->UIState, UI_Moveable,  UISet);
        {
            PushUIElement(GameState->UIState, UI_DropDownBoxParent, UISet);
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
        
        
        AddAnimationToList(GameState, &GameState->MemoryArena, Player, FindAnimation(GameState->KnightAnimations, GameState->KnightAnimationCount, "Knight_Idle"));
        
    }
    EmptyMemoryArena(&GameState->RenderArena);
    InitRenderGroup(OrthoRenderGroup, Buffer, &GameState->RenderArena, MegaBytes(1), V2(1, 1), V2(0, 0), V2(1, 1), Memory->ThreadInfo, GameState, Memory);
    
    
    
    InitRenderGroup(RenderGroup, Buffer, &GameState->RenderArena, MegaBytes(1), V2(60, 60), 0.5f*V2i(Buffer->Width, Buffer->Height), V2(1, 1), Memory->ThreadInfo, GameState, Memory);
    
    temp_memory PerFrameMemory = MarkMemory(&GameState->PerFrameArena);
    
    DebugConsole.RenderGroup = OrthoRenderGroup;
    
    rect2 BufferRect = Rect2(0, 0, (r32)Buffer->Width, (r32)Buffer->Height);
    
    v2 BufferSize = 0.5f*BufferRect.Max;
    PushClear(RenderGroup, V4(0.5f, 0.5f, 0.5f, 1));
    PushBitmap(RenderGroup, V3(0, 0, 1), &GameState->BackgroundImage, BufferRect.Max.X/MetersToPixels, BufferRect);
    // TODO(Oliver): I guess it doesn't matter but maybe push this to the ortho group once z-index scheme is in place. 
    
    Player = FindFirstEntityOfType(GameState, Entity_Player);
    
    
    switch(GameState->GameMode) {
        case MENU_MODE: {
            if(WasPressed(Memory->GameButtons[Button_Escape]))
            {
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
                        entity *Entity = InitEntity(GameState, V2i(PlayerGridP), V2(1, 1), Entity_Dropper, GameState->EntityIDAt++);
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
                    
                    // NOTE(OLIVER): This is where the player moves the block...
                    if(Chunk && Chunk->Type == ChunkBlock) {
                        v2i BlockTargetPos = GridTargetPos + PlayerMove;
                        world_chunk *NextChunk = GetOrCreateWorldChunk(GameState->Chunks, BlockTargetPos.X, BlockTargetPos.Y, 0, ChunkNull);
                        entity *Block = GetEntity(GameState, WorldChunkInMeters*V2i(GridTargetPos), Entity_Block);
                        Assert(Block && Block->Type == Entity_Block);
                        if(NextChunk && IsValidType(NextChunk, Block->ValidChunkTypes, Block->ChunkTypeCount) && !GetEntity(GameState, V2i(BlockTargetPos), Entity_Philosopher, true)) {
                            Chunk->Type = Chunk->MainType;
                            NextChunk->Type = ChunkBlock;
                            Assert(Chunk != NextChunk);
                            InitializeMove(GameState, Block, WorldChunkInMeters*V2i(BlockTargetPos));
                            
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
                            case ChunkBlock: {
                                Color01 = {0.7f, 0.3f, 0, 1};
                            } break;
                            case ChunkNull: {
                                Color01 = {0, 0, 0, 1};
                            } break;
                            default: {
                                InvalidCodePath;
                            }
                        }
                        
#if DRAW_PLAYER_PATH
                        if(IsPartOfPath(Chunk->X, Chunk->Y, &Player->Path)) {
                            Color01 = {1, 0, 1, 1};
                            PushRect(RenderGroup, Rect, 1, Color01);
                        } else 
#endif
                        {
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
            
            entity *Philosopher = FindFirstEntityOfType(GameState, Entity_Philosopher);
            for(u32 i = 0; i < GameState->CheckPointCount; i++) {
                check_point_parent *Parent = GameState->CheckPointParents + i;
                
                for(u32 j = 0; j < Parent->Count; j++) {
                    v4 Color = V4(0, 1, 0, 1);
                    if(Philosopher->CheckPointParentAt == i) {
                        if(j == Philosopher->CheckPointAt) {
                            Color= V4(1, 0, 0, 1);
                        } else {
                            Color= V4(1, 0, 1, 1);
                        }
                    }
                    check_point *CheckPoint = Parent->CheckPoints + j;
                    v2 RelPos = WorldChunkInMeters*V2i(CheckPoint->Pos) - CamPos;
                    rect2 CheckPointRect = Rect2CenterDim(RelPos, V2(0.4f, 0.4f));
                    PushRect(RenderGroup, CheckPointRect, 1, Color);
                    
                }
            }
            
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
                                
                                b32 DidMove = UpdateEntityPositionViaFunction(GameState, Entity, dt);
                                if(DidMove) {
                                    PushSound(GameState, &GameState->FootstepsSound[Entity->WalkSoundAt++], 1.0, false);
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
                                /////// NOTE(OLIVER): Player Animation
                                if(!IsEmpty(&Entity->AnimationListSentintel)) {
                                    bitmap *CurrentBitmap = GetBitmap(Entity->AnimationListSentintel.Next);
                                    
                                    PushBitmap(RenderGroup, V3(EntityRelP, 1), CurrentBitmap,  1.7f*WorldChunkInMeters, BufferRect);
                                    
                                    //// NOTE(OLIVER): Preparing to look for new animation State
#define USE_ANIMATION_STATES 0
                                    animation *NewAnimation = 0;
                                    r32 DirectionValue = 0;
                                    if(Player->Velocity.X != 0 || Player->Velocity.Y != 0) {
                                        v2 PlayerVelocity = Normal(Player->Velocity);
                                        DirectionValue = ATan2_0toTau(PlayerVelocity.Y, PlayerVelocity.X);
                                        //AddToOutBuffer("%f\n\n", DirectionValue);
                                    }
#if USE_ANIMATION_STATES
                                    r32 Qualities[ANIMATE_QUALITY_COUNT] = {};
                                    r32 Weights[ANIMATE_QUALITY_COUNT] = {1.0f};
                                    
                                    Qualities[DIRECTION] = DirectionValue; 
                                    
                                    NewAnimation = GetAnimation(GameState->KnightAnimations, GameState->KnightAnimationCount, Qualities, Weights);
#else 
                                    char *AnimationName = 0;
                                    r32 Speed = Length(Player->Velocity);
                                    AddToOutBuffer("%f\n", Speed);
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
                            }
                            
                            ///////
                        } break;
                        case Entity_Door: {
                            v2i EntityPos = GetGridLocation(Entity->Pos);
                            GetOrCreateWorldChunk(GameState->Chunks, EntityPos.X, EntityPos.Y, 0, ChunkNull);
                            
                            
                            
                            //PushRect(RenderGroup, EntityAsRect, 1, V4(0, 1, 1, 1));
                            
                            PushBitmap(RenderGroup, V3(EntityRelP, 1), &GameState->Door,  1.1f*WorldChunkInMeters, BufferRect);
                            
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
                                
                                b32 CanSeePlayer = ContainsChunkType(Entity, PlayerChunk->Type);
                                
                                if(LookingForPosition(Entity, PlayerPos) && !CanSeePlayer) {
                                    EndPath(Entity);
                                }
                                
                                if(Entity->LastSearchPos != PlayerPos && CanSeePlayer && Entity->IsAtEndOfMove) {
                                    v2 TargetPos_r32 = WorldChunkInMeters*V2i(PlayerPos);
                                    
                                    WasSuccessful = InitializeMove(GameState, Entity, TargetPos_r32);
                                    if(WasSuccessful) {
                                        Entity->LastSearchPos = PlayerPos;
                                    }
                                } 
                                //Then if we couldn't find the player we just move a random direction
                                if(!WasSuccessful && !HasMovesLeft(Entity)) {
                                    if(Entity->CheckPointParentAt) {
                                        check_point_parent *Parent = GameState->CheckPointParents + Entity->CheckPointParentAt;
                                        check_point *CurrentCheckPoint = Parent->CheckPoints + Entity->CheckPointAt;
                                        
                                        v2 TargetPos_r32 = WorldChunkInMeters*V2i(CurrentCheckPoint->Pos);
                                        b32 SuccessfulMove = InitializeMove(GameState, Entity, TargetPos_r32);
                                        Assert(SuccessfulMove);
                                        
                                        Entity->CheckPointAt++;
                                        if(Entity->CheckPointAt >= Parent->Count) {
                                            Entity->CheckPointAt = 0;
                                        }
                                    }
#define RANDOM_WALK 0
#if RANDOM_WALK
                                    
                                    random_series *RandGenerator = &GameState->GeneralEntropy;
                                    
                                    s32 PossibleStates[] = {-1, 0, 1};
                                    v2i Dir = {};
                                    for(;;) {
                                        b32 InArray = false;
                                        fori(Entity->LastMoves) {
                                            if(Dir == Entity->LastMoves[Index]) {
                                                InArray = true;
                                                break;
                                            }
                                        }
                                        if(InArray) {
                                            s32 Axis = RandomBetween(RandGenerator, 0, 1);
                                            if(Axis) {
                                                Dir.X = RandomBetween(RandGenerator, -1, 1);
                                                Dir.Y = 0;
                                                
                                            } else {
                                                Dir.X = 0;
                                                Dir.Y = RandomBetween(RandGenerator, -1, 1);
                                            }
                                            
                                        } else {
                                            break;
                                        }
                                    }
                                    Entity->LastMoves[Entity->LastMoveAt++] = V2int(-Dir.X,-Dir.Y);
                                    if(Entity->LastMoveAt >= ArrayCount(Entity->LastMoves)) { Entity->LastMoveAt = 0;}
                                    
                                    v2 TargetPos_r32 = WorldChunkInMeters*V2i(GetGridLocation(Entity->Pos) + Dir);
                                    b32 SuccessfulMove = InitializeMove(GameState, Entity, TargetPos_r32);
                                    Assert(SuccessfulMove);
#endif //Random walk 
                                    
                                }
                                
                                UpdateEntityPositionViaFunction(GameState, Entity, dt);
                                if(GetGridLocation(Entity->Pos) == GetGridLocation(Player->Pos)) {
                                    GameState->GameMode = GAMEOVER_MODE;
                                }
                            }
#if 0
                            PushRect(RenderGroup, EntityAsRect, 1, V4(1, 0, 0, 1));
#else 
                            PushBitmap(RenderGroup, V3(EntityRelP, 1), &GameState->Monster,  1.0f*WorldChunkInMeters, BufferRect);
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
                            b32 Moved = UpdateEntityPositionViaFunction(GameState, Entity, dt);
                            if(Moved) {
                                entity* Changer = InitEntity(GameState, Entity->Pos, WorldChunkInMeters*V2(0.5f, 0.5f), Entity_Chunk_Changer, GameState->EntityIDAt++);
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
#define PushOntoLastElms(LastElms, LastElmCount, ChildElm)  Assert(LastElmCount < ArrayCount(LastElms)); CurrentElm = LastElms[LastElmCount++] = ChildElm;
                
                u32 ArrayCount = 0; 
                u32 IndexesProcessed[ArrayCount(GameState->UIState->Elements)] = {};
                v2 PosAt = {};
                ui_element *NextHotEntity = 0;
                r32 IndentSpace = 20;
                u32 LastElmCount = 0;
                ui_element *LastElms[64] = {};
                r32 MenuWidth = 300;
                v2 StartP = {};
                
                for(u32 UIIndex = 0; UIIndex < GameState->UIState->ElmCount; ++UIIndex) {
                    ui_element *Element = GameState->UIState->Elements + UIIndex;
                    
                    //loop through all ui elements
                    if(!IndexesProcessed[UIIndex] && Element->IsValid) { //Check if it has been drawn
                        Assert(LastElmCount == 0);
                        StartP = PosAt = Element->Set.Pos;
                        
                        rect2 ClipRect = Rect2MinDim(StartP, V2(MenuWidth, StartP.Y + MAX_S32));
                        
                        ui_element *CurrentElm = 0;
                        
                        PushOntoLastElms(LastElms, LastElmCount, Element);
                        
                        for(;;) {
                            Assert(LastElmCount > 0);
                            CurrentElm = LastElms[LastElmCount - 1];
                            
                            for(u32 ChildAt = CurrentElm->TempChildrenCount; 
                                ChildAt <  CurrentElm->ChildrenCount; 
                                ) {
                                b32 AdvanceChildIndex = true;
                                u32 ChildIndex = CurrentElm->ChildrenIndexes[ChildAt];
                                
                                IndexesProcessed[ChildIndex] = true;
                                
                                ui_element *ChildElm = &GameState->UIState->Elements[ChildIndex];
                                
                                Assert(ChildElm->IsValid);
                                
                                rect2 ElementAsRect = {};
                                switch(ChildElm->Type) {
                                    case UI_Moveable:{
                                        ElementAsRect = Rect2MinMax(V2(PosAt.X, PosAt.Y - GameState->DebugFont->LineAdvance), V2(PosAt.X + MenuWidth, PosAt.Y));
                                        PushRectOutline(OrthoRenderGroup, ElementAsRect, 1, V4(0.5f, 1, 0, 1)); 
                                        PosAt.Y -= GetHeight(ElementAsRect);
                                    } break;
                                    case UI_DropDownBoxParent: {
                                        if(ChildElm->Set.Active) {
                                            if(ChildElm->ChildrenCount) {
#if 0
                                                u32 DropDownChildIndex = ChildElm->ChildrenIndexes[ChildElm->Set.ActiveChildIndex];
                                                ui_element *ActiveElm = GameState->UIState->Elements + DropDownChildIndex;
#endif
                                                DrawMenuItem(ChildElm, OrthoRenderGroup, &PosAt, GameState->DebugFont, &ElementAsRect, ClipLeftX(PosAt, ClipRect));
                                            }
                                        }
                                    } break;
                                    case UI_DropDownBox: {
                                        Assert(CurrentElm->Type == UI_DropDownBoxParent);
                                        if(!CurrentElm->Set.Active) {
                                            DrawMenuItem(ChildElm, OrthoRenderGroup, &PosAt, GameState->DebugFont, &ElementAsRect, ClipLeftX(PosAt, ClipRect));
                                        }
                                        
                                    } break;
                                    case UI_CheckBox: {
                                        DrawMenuItem(ChildElm, OrthoRenderGroup, &PosAt, GameState->DebugFont, &ElementAsRect, ClipLeftX(PosAt, ClipRect));
                                    } break;
                                    case UI_Button: {
                                        DrawMenuItem(ChildElm, OrthoRenderGroup, &PosAt, GameState->DebugFont, &ElementAsRect, ClipLeftX(PosAt, ClipRect));
                                    } break;
                                    case UI_Entity: {
                                        //Already Rendered Above
                                        u32 ID = CastVoidAs(u32, ChildElm->Set.ValueLinkedToPtr);
                                        entity *Entity = FindEntityFromID(GameState, ID);
                                        v2 EntityRelP = Entity->Pos - CamPos;
                                        
                                        ElementAsRect = Rect2CenterDim(EntityRelP, Entity->Dim);
                                        if(InBounds(ElementAsRect, MouseP_PerspectiveSpace)) {
                                            NextHotEntity = ChildElm;
                                            //AddToOutBuffer("Is Hot!\n");
                                        }
                                        
                                    } break;
                                }
                                
                                ChildElm->Set.Pos = ElementAsRect.Min;
                                ChildElm->Set.Dim = ElementAsRect.Max - ElementAsRect.Min;
                                
                                if(InBounds(ElementAsRect, MouseP_OrthoSpace)) {
                                    NextHotEntity = ChildElm;
                                }
                                
                                if(ChildElm->ChildrenCount) {
                                    PosAt.X += IndentSpace;
                                    CurrentElm->TempChildrenCount = ChildAt + 1;
                                    PushOntoLastElms(LastElms, LastElmCount, ChildElm);
                                    AdvanceChildIndex = false;
                                } 
                                if(AdvanceChildIndex) {
                                    ++ChildAt;
                                }
                            }
                            CurrentElm->TempChildrenCount = 0;
                            
                            if(LastElmCount > 1) {
                                PosAt.X -= IndentSpace;
                                LastElmCount--;
                            } else {
                                LastElmCount--;
                                // NOTE(Oliver): Finshed rendering whole ui structure
                                break; 
                            }
                        }
                    }
                }
                
                //PushRectOutline(OrthoRenderGroup, Rect2MinMax(V2(0, 0),V2(100, 100)), 1, V4(0, 0, 0, 1)); 
                
                //AddToOutBuffer("{%f, %f}", MouseP_OrthoSpace.X, MouseP_OrthoSpace.Y);
                
                ///////////////
                ui_state *UIState = GameState->UIState;
                if(WasPressed(Memory->GameButtons[Button_LeftMouse])) {
                    //Add Block
                    if(IsDown(Memory->GameButtons[Button_Shift])) {
                        v2i Cell = GetGridLocation(MouseP_PerspectiveSpace + Camera->Pos);
                        world_chunk *Chunk = GetOrCreateWorldChunk(GameState->Chunks, Cell.X, Cell.Y, 0, ChunkNull);
                        
                        enum_array_data *Info = &UIState->InitInfo;
                        char *A = Info->Type;
                        if(A) {
                            if(Chunk) {
                                
                                if(DoStringsMatch(A, "entity_type")) {
                                    CastValue(Info, entity_type);
                                    v2 Pos = WorldChunkInMeters*V2i(Cell);
                                    entity *Ent = GetEntityAnyType(GameState, Pos);
                                    if(Ent) {
                                        if(Ent->Type != Entity_Camera && Ent->Type != Entity_Player) {
                                            if(Ent->Type == Entity_Block) {
                                                Chunk->Type = Chunk->MainType;
                                            }
                                            RemoveEntity(GameState, Ent->Index);
                                        }
                                    } else {
                                        if(Value == Entity_Chunk_Changer) {
                                            entity *Entity = InitEntity(GameState, Pos, V2(0.4f, 0.4f), Entity_Chunk_Changer, GameState->EntityIDAt++);
                                            Entity->LoopChunks = true;
                                            AddChunkTypeToChunkChanger(Entity, ChunkLight);
                                            AddChunkTypeToChunkChanger(Entity, ChunkDark);
                                            AddChunkTypeToChunkChanger(Entity, ChunkLight);
                                            AddChunkTypeToChunkChanger(Entity, ChunkDark);
                                            AddChunkTypeToChunkChanger(Entity, ChunkLight);
                                            AddChunkTypeToChunkChanger(Entity, ChunkDark);
                                            AddChunkTypeToChunkChanger(Entity, ChunkLight);
                                            AddChunkTypeToChunkChanger(Entity, ChunkDark);
                                        } else {
                                            AddEntity(GameState, Pos, Value);
                                        }
                                    }
                                } 
                            } 
                            
                            if(DoStringsMatch(A, "chunk_type")) {
                                CastValue(Info, chunk_type);
                                
                                if(Chunk && Chunk->Type == Value) {
                                    RemoveWorldChunk(GameState->Chunks, Cell.X, Cell.Y, &GameState->ChunkFreeList);
                                } else {
                                    if(!Chunk) {
                                        Assert(Value != ChunkNull);
                                        Chunk = GetOrCreateWorldChunk(GameState->Chunks, Cell.X, Cell.Y, &GameState->MemoryArena, Value, &GameState->ChunkFreeList);
                                    } 
                                    
                                    Chunk->Type = Value;
                                }
                            }
                        } else {
                            AddToOutBuffer("No Entity type selected\n");
                        }
                    } else { //Picking up entities with mouse. 
                        
                        if(!UIState->InteractingWith) {
                            if(!NextHotEntity) {
#if MOVE_VIA_MOUSE
                                //NOTE: This is where we move the player.
                                v2 TargetP_r32 = MouseP_PerspectiveSpace + GameState->Camera->Pos;
                                InitializeMove(GameState, Player, TargetP_r32);
#endif
                            } else {
                                //NOTE: This is interactable objects;
                                Assert(NextHotEntity->IsValid);
                                UIState->HotEntity = NextHotEntity;
                                UIState->InteractingWith = UIState->HotEntity;
                                if(UIState->InteractingWith->Type == UI_Entity) {
                                    GetEntityFromElement(UIState);
                                    Entity->RollBackPos = Entity->Pos;
                                }
                            }
                        }
                    }
                }
                if(WasReleased(Memory->GameButtons[Button_LeftMouse])) {
                    
                    //MOUSE BUTTON RELEASED ACTIONS
                    if(UIState->InteractingWith) {
                        ui_element *InteractEnt = UIState->InteractingWith;
                        switch(InteractEnt->Type) {
                            case UI_DropDownBox: {
                                ui_element *Parent = InteractEnt->Parent;
                                
                                //VIEW
                                Parent->Set.Name = InteractEnt->Set.Name;
                                
                                //DATA
                                enum_array_data *ValueToMod = (enum_array_data *)InteractEnt->Set.ValueLinkedToPtr;
                                
                                *ValueToMod = InteractEnt->Set.EnumArray;
                                
                                //Show Active
                                Parent->Set.Active = !Parent->Set.Active;
                            } break;
                            case UI_Moveable: {
                                
                            } break;
                            case UI_Button: {
                                // TODO(Oliver): Can we make this more generic?
                                if(DoStringsMatch(InteractEnt->Set.Type, "Save Level")) {
                                    SaveLevelToDisk(Memory, GameState, "level1.omm");
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
                            case UI_DropDownBoxParent: {
                                InteractEnt->Set.Active = !InteractEnt->Set.Active;
                            } break;
                            default: {
                                AddToOutBuffer("There is no release interaction for the object type \"%s\"\n", element_ui_type_Names[InteractEnt->Type]);
                            }
                        }
                        
                        UIState->InteractingWith = 0;
                    }
                }
                
                if(UIState->InteractingWith) {
                    //MOUSE BUTTON DOWN ACTIONS
                    
                    ui_element *Interact = UIState->InteractingWith;
                    switch(UIState->InteractingWith->Type) {
                        case UI_Entity: {
                            GetEntityFromElement(UIState);
                            if(Entity->Type == Entity_Block) {
                                SetChunkType(GameState, Entity->Pos, ChunkMain);
                                
                                Entity->Pos = WorldChunkInMeters*GetGridLocationR32(MouseP_PerspectiveSpace + CamPos);
                                
                                
                                SetChunkType(GameState, Entity->Pos, ChunkBlock);
                            } else {
                                AddToOutBuffer("There is no down interaction for the object type \"%s\"\n", entity_type_Names[Entity->Type]);
                            }
                            
                        } break;
                        case UI_Moveable:{
                            Interact->Parent->Set.Pos = MouseP_OrthoSpace;
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
                        ElmAsRect = Rect2MinDim(ElmRelP, Entity->Dim);
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
                            ElmAsRect = Rect2MinDim(ElmRelP, Entity->Dim);
                        } 
                        
                        PushRectOutline(ThisRenderGroup, ElmAsRect, 1, V4(0.2f, 0, 1, 1)); 
                    }
                } else {
                    v2 Cell = GetGridLocationR32(MouseP_PerspectiveSpace + Camera->Pos);
                    
                    v2 ElmRelP = WorldChunkInMeters*Cell - CamPos;
                    
                    rect2 ElmAsRect = Rect2MinDim(ElmRelP, V2(WorldChunkInMeters, WorldChunkInMeters));
                    
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
            
            PushBitmap(OrthoRenderGroup, V3(MouseP_OrthoSpace, 1), &GameState->MagicianHandBitmap, 100.0f, BufferRect);
            
            ///////////////////
            
        } //END OF PLAY_MODE 
    } //switch on GAME_MODE
    
    
    //RenderGroupToOutput(RenderGroup);
    //RenderGroupToOutput(&OrthoRenderGroup);
    
    ReleaseMemory(&PerFrameMemory);
    
}

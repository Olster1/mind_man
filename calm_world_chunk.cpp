
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

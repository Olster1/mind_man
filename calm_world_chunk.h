
struct world_chunk {
    s32 X;
    s32 Y;
    
    chunk_type Type;
    chunk_type MainType;
    
    world_chunk *Next;
};

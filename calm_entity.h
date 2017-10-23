
enum entity_type
{
    Entity_Null,
    Entity_Block, 
    Entity_Guru, 
    Entity_Philosopher,
    Entity_Player,
    Entity_Chunk_Changer,
    Entity_Camera,
    Entity_Dropper,
    Entity_Home,
    Entity_Door,
    Entity_CheckPoint,
    Entity_CheckPointParent,
    ///////
    Entity_Count,
};

enum search_type {
    SEARCH_VALID_SQUARES,
    SEARCH_INVALID_SQUARES,
    SEARCH_FOR_TYPE,
};

struct search_cell {
    v2i Pos;
    v2i CameFrom;
    
    search_cell *Prev;
    search_cell *Next;
};

struct path_nodes {
    v2 Points[512];
    u32 Count;
    loaded_sound *Sound;
};

Introspect struct entity { 
    u32 ID;  //we no longer identify by pointers _or_ indexes, we search for IDs
    
    v2 Pos;
    v2 Velocity;
    v2 Dim;
    
    // NOTE(Oliver): This is for the chunk changer
    u32 ChunkAt;
    u32 ChunkListCount;
    chunk_type ChunkList[16];
    timer ChunkTimer; 
    b32 LoopChunks;
    //
    
    u32 WalkSoundAt; //For Sound effects
    
    r32 LifeSpan;//seconds
    
    //NOTE(oliver): This is used by the camera only at the moment 1/5/17
    v2 AccelFromLastFrame; 
    //
    
    //For UI editor. Maybe move it to the UI element struct. Oliver 13/8/17 
    v2 RollBackPos;
    //
    r32 Time;
    //particle_system ParticleSystem;
    
    // NOTE(OLIVER): this is for the philosophers checkpoints
    //SAVED
    //Maybe combine these with the checkpoints used by the check point parents
    u32 CheckPointParentCount; 
    u32 CheckPointParentIds[32];
    //
    u32 CheckPointParentAt; // Find this via a search
    u32 CheckPointAt; //For looping through
    //
    
    // NOTE(OLIVER): This is for the check_points 
    loaded_sound *SoundToPlay;
    //
    
    // NOTE(OLIVER): This is for the check_point parents
    u32 CheckPointCount;
    u32 CheckPointIds[16];
    //
    
    // NOTE(OLIVER): This is for the philosopher AI random walk
    v2i LastMoves[1];
    u32 LastMoveAt;
    v2i LastSearchPos;
    //
    
    animation_list_item AnimationListSentintel;
    
    u32 VectorIndexAt;
    u32 ChunkTypeCount;
    chunk_type ValidChunkTypes[16];
    
    path_nodes Path;
    
    b32 IsAtEndOfMove;
    
    v2 EndOffsetTargetP;
    v2 BeginOffsetTargetP;
    
    r32 MoveT;
    r32 MovePeriod;
    
    s32 LifePoints;
    mat2 Rotation;
    r32 FacingDirection;    
    
    b32 TriggerAction;
    entity_type Type;
    b32 Moves;
    
    r32 InverseWeight;
    b32 Collides; 
    u32 Index;
};

struct line
{
    v2 Begin;
    v2 End;
};

#if 0
// NOTE(OLIVER): This is old movement code for the player. I probably won't need it in the future but just in case. 13/2/17
#if MOVE_VIA_ACCELERATION
v2 RelVector = Entity->Pos - Entity->StartPos; 

r32 TValue = Length(RelVector) / Length(Entity->TargetPos - Entity->StartPos); 

//PrintToConsole(&GameState->Console, );
r32 AccelFactor = 0;
if(TValue < 0.5f) {
    AccelFactor= SineousLerp0To1(500, TValue, 300);
} else {
    AccelFactor = SineousLerp0To1(300, TValue, 100);
}

Acceleration = AccelFactor*Normal(Entity->TargetPos - Entity->Pos);
UpdateEntityPositionWithImpulse(GameState,Entity, dt, Acceleration);
if(TValue >= 1.0f) {
    Entity->Pos = Entity->TargetPos;
    Entity->Velocity = {};
}
#else
//UpdateEntityPosViaFunction(Entity, dt);
#endif

#endif
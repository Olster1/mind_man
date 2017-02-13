
enum entity_type
{
    Entity_Null,
    Entity_Object, 
    Entity_Player,
    Entity_Camera,
    
};

struct entity
{
    v2 Pos;
    v2 Velocity;
    v2 Dim;
    
    v2 StartPos;
    
    v2 TempTargetPos;
    
    v2 TargetPos;
    v2 OffsetTargetP;
    r32 MoveT;
    r32 MovePeriod;
    
    s32 LifePoints;
    mat2 Rotation;
    r32 FacingDirection;    
    
    b32 Moving;
    
    u32 Type;
    b32 Moves;
    
    r32 InverseWeight;
    
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
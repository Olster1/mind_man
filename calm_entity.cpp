
struct array_v2
{
    v2 *Ptr;
    u32 Count;
};

inline b32
IsRemoved(u32 Index, u32 RemoveBitField)
{
    b32 Result = ((1 << Index) & RemoveBitField);
    return Result;
}

internal array_v2
MinkowskiSumConvexHull(memory_arena *Arena, v2 *VertexA, u32 CountA, v2 *VertexB, u32 CountB, r32 SumBias)
{
    
    u32 Count = 0;
    u32 NumberOfPoints = CountA*CountB;
    v2 *TempResult = PushArray(Arena, v2, NumberOfPoints);
    
    //NOTE(oliver): Do Minkowsi Sum
    forN(CountA)
    {
        forN(CountB)
        {
            //TODO(ollie): Think more deeply about sum bias
            TempResult[Count++] = VertexA[CountAIndex] + VertexB[CountBIndex];
        }
    }
    
    //NOTE(oliver): Find furtherest vector from origin
    v2 *MaxVertex = 0;
    r32 MaxLengthSqr = 0;
    forN(NumberOfPoints)
    {
        v2 Vertex = TempResult[NumberOfPointsIndex];
        r32 VertexLengthSqr = LengthSqr(Vertex);
        
        if(VertexLengthSqr > MaxLengthSqr)
        {
            MaxLengthSqr = VertexLengthSqr;
            MaxVertex = TempResult + NumberOfPointsIndex;
        }
    }
    
    Assert(MaxVertex);
    
    //NOTE(oliver): Find vectors that are not on the surface
    u32 RemoveBitField = 0;
    v2 *LastMaxVertex = 0;
    v2 *BeginMaxVertex = MaxVertex;
    r32 RadiansRange = 0;
    
    array_v2 Result = {};
    Result.Ptr = PushArray(Arena, v2, NumberOfPoints);
    
    for(;;)
    {
        v2 *MaxTestVertex = 0;
        v2 *OtherSideMaxTestVertex = 0;
        
        r32 MaxShadow = -FLT_MAX;
        r32 OtherSideMaxShadow = FLT_MAX;
        v2 MaxVertexPerp = Perp(*MaxVertex);
        r32 MaxLineOfSightSqr = 0.0f;
        r32 OtherSideMaxLineOfSightSqr = 0.0f;
        u32 MaxTestVertexIndex = 0;
        u32 OtherSideMaxTestVertexIndex = 0;
        
        
        forN(NumberOfPoints)
        {
            v2 *TestVertex = TempResult + NumberOfPointsIndex;
            
            if(TestVertex != MaxVertex && TestVertex != LastMaxVertex && !IsRemoved(NumberOfPointsIndex, RemoveBitField))
            {
                if(LastMaxVertex)
                {
                    r32 RadiansBetween = InverseCos(Inner(Normal(*LastMaxVertex), Normal(*TestVertex)));
                    RadiansBetween = (Inner(Perp(*LastMaxVertex), *TestVertex) > 0.0f) ? (PI32 + PI32 - RadiansBetween) : RadiansBetween;
                    
                    if(RadiansBetween < RadiansRange)
                    {
                        RemoveBitField |= (1 << NumberOfPointsIndex);
                    }
                }
                
                if(!IsRemoved(NumberOfPointsIndex, RemoveBitField))
                {
                    v2 LineOfSight = *TestVertex - *MaxVertex;
                    r32 TestShadow = Inner(*MaxVertex, Normal(LineOfSight));
                    r32 LineOfSightLengthSqr = LengthSqr(LineOfSight);
                    
                    if(Inner(MaxVertexPerp, *TestVertex) <= 0.0f)
                    {
                        
                        b32 EqualAndLonger = false;
                        if(TestShadow == MaxShadow)
                        {
                            EqualAndLonger = (LineOfSightLengthSqr > MaxLineOfSightSqr);
                        }
                        
                        if(TestShadow > MaxShadow || EqualAndLonger)
                        {
                            MaxShadow = TestShadow;
                            MaxTestVertex = TestVertex;
                            MaxLineOfSightSqr = LineOfSightLengthSqr;
                            MaxTestVertexIndex = NumberOfPointsIndex;
                        }
                    }
                    else
                    {
                        b32 EqualAndLonger = false;
                        if(TestShadow == OtherSideMaxShadow)
                        {
                            EqualAndLonger = (LineOfSightLengthSqr > OtherSideMaxLineOfSightSqr);
                        }
                        
                        if(TestShadow < OtherSideMaxShadow || EqualAndLonger)
                        {
                            OtherSideMaxShadow = TestShadow;
                            OtherSideMaxTestVertex = TestVertex;
                            OtherSideMaxLineOfSightSqr = LineOfSightLengthSqr;
                            OtherSideMaxTestVertexIndex = NumberOfPointsIndex;
                        }
                    }
                }
            }
        }
        
        if(LastMaxVertex && MaxVertex == BeginMaxVertex)
        {
            break;
        }
        
        Assert(MaxTestVertex || OtherSideMaxTestVertex);
        
        Result.Ptr[Result.Count++] = *MaxVertex;
        LastMaxVertex = MaxVertex;
        
        MaxVertex = (MaxTestVertex) ? MaxTestVertex : OtherSideMaxTestVertex;
        u32 IndexToRemove = (MaxTestVertex) ? MaxTestVertexIndex : OtherSideMaxTestVertexIndex;
        RemoveBitField |= (1 << IndexToRemove);
        
        
        r32 CosValue = Inner(Normal(*LastMaxVertex), Normal(*MaxVertex));
        Assert(CosValue <= 1.0f && CosValue >= -1.0f);        
        RadiansRange = InverseCos(CosValue);
        Assert(RadiansRange >= 0.0f);
        RadiansRange = (Inner(Perp(*LastMaxVertex), *MaxVertex) > 0.0f) ? (TAU32 - RadiansRange) : RadiansRange;
    }
    
    return Result;
    
}


internal array_v2
MinkowskiSum(memory_arena *Arena, entity *EntityA, entity *EntityB, r32 SumBias)
{
    rect2 A = Rect2OriginCenterDim(EntityA->Dim);
    rect2 B = Rect2OriginCenterDim(EntityB->Dim);
    
    v2 VertexA[4] = {A.Min, V2(A.Min.X, A.Max.Y), V2(A.Max.X, A.Min.Y), A.Max};
    
    v2 OriginOffset = EntityB->Pos;
    
    u32 VertexBCount = 0;
    v2 *VertexB = 0;
    
    
    if(EntityB->Type == Entity_Player)
    {
        VertexBCount = 4;
        VertexB = PushArray(Arena, v2, VertexBCount);
        VertexB[0] = B.Min;
        VertexB[1] = V2(B.Min.X, B.Max.Y);
        VertexB[2] = V2(B.Max.X, B.Min.Y);
        VertexB[3] = B.Max;
        
        fori_count(VertexBCount)
        {
            VertexB[Index] = Multiply(VertexB[Index], EntityB->Rotation);
        }
        
    } else {
        InvalidCodePath;
    }
    
    fori(VertexA)
    {
        VertexA[Index] = Multiply(VertexA[Index], EntityA->Rotation);
    }
    
    Assert(VertexA);
    Assert(VertexB);
    
    array_v2 Result = MinkowskiSumConvexHull(Arena, VertexA, ArrayCount(VertexA), VertexB, VertexBCount, SumBias);
    
    for(u32 Index = 0; Index < Result.Count; ++Index)
    {
        Result.Ptr[Index] += OriginOffset;
    }
    
    return Result;
}

struct collision_info
{
    v2 SurfaceNormal;
    entity *HitEntity;
};

internal v2 *
CreateSides(memory_arena *Arena, v2 *Vertex, u32 Count)
{
    
    v2 *Sides = PushArray(Arena, v2, Count);
    
    for(u32 SideIndex = 0; SideIndex < Count; ++SideIndex)
    {
        u32 NextIndex = SideIndex + 1;
        if(NextIndex == Count)
        {
            NextIndex = 0;
        }
        
        Sides[SideIndex] = Vertex[NextIndex] - Vertex[SideIndex];
    }
    
    return Sides;
}

inline b32
IsOverlapping(array_v2 Vertexes, v2 P)
{
    b32 Result = true;
    u32 Count = Vertexes.Count;
    
    forN(Count)
    {
        v2 RelP = P - Vertexes.Ptr[CountIndex];
        
        u32 CanonicalCount = CountIndex + 1;
        if(CanonicalCount == Count) { CanonicalCount = 0; }
        
        v2 Side = Vertexes.Ptr[CanonicalCount] - Vertexes.Ptr[CountIndex];
        
        if(Inner(Perp(Side), RelP) >= 0.0f)
        {
            Result = false;
            break;
        }
    }
    
    return Result;
}

internal void
UpdateEntityPositionWithImpulse(game_state *GameState, entity *Entity, r32 dt, v2 Acceleration, b32 Collides = true)
{
    
    r32 DragCoeff = 0.4f;
    v2 EntityNewPos = 0.5f*Sqr(dt)*Acceleration + dt*Entity->Velocity + Entity->Pos;
    Entity->Velocity += dt*Acceleration - DragCoeff*Entity->Velocity;
    
    
    v2 DeltaEntityPos = EntityNewPos - Entity->Pos;
    
    if(Collides)
    {
        for(u32 CollisionLoopCount = 0; CollisionLoopCount < 4; ++CollisionLoopCount)
        {
            r32 MinT = 1.0f;
            
            u32 CollisionInfoCount = 0;
            collision_info CollisionInfo[1];
            
            for(u32 EntityIndex = 0; EntityIndex < GameState->EntityCount; ++EntityIndex)
            {
                entity *TestEntity = GameState->Entities + EntityIndex;
                if(TestEntity != Entity)
                {
                    array_v2 VertexPoints = MinkowskiSum(&GameState->PerFrameArena, Entity, TestEntity, TestEntity->FacingDirection);
                    
                    v2 *Vertex = VertexPoints.Ptr;
                    
                    v2 *Sides = CreateSides(&GameState->PerFrameArena, Vertex, VertexPoints.Count);
                    
                    //Assert(!Overlapping(VertexPoints.Ptr, Sides, VertexPoints.Count, Entity->Pos));
                    
                    for(u32 VertexIndex = 0; VertexIndex < VertexPoints.Count; ++VertexIndex)
                    {
                        v2 Side = Sides[VertexIndex];
                        v2 NormalSide = Normal(Side);
                        v2 RelPToVertex = Entity->Pos - Vertex[VertexIndex];
                        
                        v2 ObjectSpaceXAxis = NormalSide;
                        v2 ObjectSpaceYAxis = Perp(NormalSide);
                        
                        v2 ObjectSpaceEntityPos = V2(Inner(RelPToVertex, ObjectSpaceXAxis), Inner(RelPToVertex, ObjectSpaceYAxis));
                        v2 ObjectSpaceDeltaEntityPos = V2(Inner(DeltaEntityPos, ObjectSpaceXAxis), Inner(DeltaEntityPos, ObjectSpaceYAxis));
                        
                        r32 t = (-ObjectSpaceEntityPos.Y) / ObjectSpaceDeltaEntityPos.Y;
                        
                        if(t <= MinT && t >= 0.0f)
                        {
                            r32 XIntercept = ObjectSpaceEntityPos.X + t*ObjectSpaceDeltaEntityPos.X;
                            
                            r32 SideLength = Length(Side);
                            if(XIntercept >= 0.0f && XIntercept < SideLength)
                            {
                                MinT = t;
                                
                                collision_info Info = {};
                                Info.SurfaceNormal = TestEntity->FacingDirection*Perp(NormalSide);
                                Info.HitEntity = TestEntity;
                                
                                if(t < MinT)
                                {
                                    CollisionInfoCount = 0;
                                }
                                
                                if(CollisionInfoCount < ArrayCount(CollisionInfo))
                                {
                                    CollisionInfo[CollisionInfoCount++] = Info;
                                }
                                
                                Assert(MinT >= 0.0f && MinT <= 1.0f);
                            }
                        }
                    }
                }
            }
            
            r32 tEpsilon = 0.01f;
            MinT = Max(MinT - tEpsilon, 0.0f);
            
            Entity->Pos += MinT*DeltaEntityPos;
            DeltaEntityPos = (1.0f - MinT)*DeltaEntityPos;
            
            v2 CollisionImpulse = {};
            v2 TestEntityCollisionImpulse = {};
            v2 SurfaceNormal = {};
            
            if(CollisionInfoCount)
            {
                collision_info Info = CollisionInfo[0];
                entity *HitEntity = Info.HitEntity;
                
                SurfaceNormal = Info.SurfaceNormal;
                
                Assert(HitEntity->LifePoints != 0);
                if(HitEntity->LifePoints > 0)
                {
                    --HitEntity->LifePoints;
                    
                    if(HitEntity->LifePoints == 0)
                    {
                        u32 IndexToRemove = HitEntity->Index;
                        GameState->Entities[IndexToRemove] = GameState->Entities[--GameState->EntityCount];
                        GameState->Entities[IndexToRemove].Index = IndexToRemove;
                    }
                }
                
                v2 VelRel = Entity->Velocity - HitEntity->Velocity;
                r32 ReboundCoefficient = 1.0f;
                
                r32 Weight = 100.0f;
                
                r32 K = Inner(SurfaceNormal, (1 + ReboundCoefficient) * VelRel) /
                    Inner((Entity->InverseWeight + HitEntity->InverseWeight)* SurfaceNormal, SurfaceNormal);
                
                CollisionImpulse = -(K * Entity->InverseWeight)*SurfaceNormal;
                v2 HitEntityCollisionImpulse = (K * HitEntity->InverseWeight)*SurfaceNormal;
                
                HitEntity->Velocity += HitEntityCollisionImpulse;
                Entity->Velocity += CollisionImpulse;
                
                DeltaEntityPos += dt*CollisionImpulse;
                
            }
            
        }
    }
    else
    {
        Entity->Pos = EntityNewPos;
    }
    
}

inline entity *
InitEntity(game_state *GameState, v2 Pos, v2 Dim, entity_type Type)
{
    Assert(GameState->EntityCount < ArrayCount(GameState->Entities));
    
    u32 EntityIndex = GameState->EntityCount++;
    entity *Entity = &GameState->Entities[EntityIndex];
    
    *Entity = {};
    Entity->Velocity = {};
    Entity->MovePeriod = 4.0f;
    Entity->Pos = Entity->StartPos = Entity->TargetPos = Pos;
    Entity->Dim = Dim;
    Entity->Index = EntityIndex;
    Entity->LifePoints = -1;
    Entity->Rotation = Mat2();
    Entity->FacingDirection = 1;
    Entity->Type = Type;
    
    return Entity;
}

internal void
UpdateRotation(game_state *GameState, entity *Entity, r32 Value)
{
    mat2 NewRotation = Rotate(Entity->Rotation, Value);
    
    b32 Overlapping = false;
    for(u32 EntityIndex = 0; EntityIndex < GameState->EntityCount; ++EntityIndex)
    {
        entity *TestEntity = GameState->Entities + EntityIndex;
        if(TestEntity != Entity)
        {
            array_v2 VertexPoints = MinkowskiSum(&GameState->PerFrameArena, Entity, TestEntity, TestEntity->FacingDirection);
            
            v2 *Vertex = VertexPoints.Ptr;
            
            v2 *Sides = CreateSides(&GameState->PerFrameArena, Vertex, VertexPoints.Count);
            
            if(IsOverlapping(VertexPoints, Entity->Pos))
            {
                Overlapping  = true;
                break;
            }
        }
    }
    if(!Overlapping)
    {
        Entity->Rotation = NewRotation;
    }
}

#if 0
internal void
SnapToNearestLine(game_state *GameState)
{
    entity *InteractingWith = GameState->InteractingWith;
    r32 DistanceToSnapSqr = 1000.0f;
    
    fori_count(GameState->LineCount)
    {
        entity *TestLine = GameState->Entities + GameState->CachedLinesIndex[Index];
        if(InteractingWith != TestLine)
        {
            if(LengthSqr(TestLine->End - InteractingWith->End) < DistanceToSnapSqr)
            {
                InteractingWith->End = TestLine->End;
                break;
            }
            if(LengthSqr(TestLine->Begin - InteractingWith->End) < DistanceToSnapSqr)
            {
                InteractingWith->End = TestLine->Begin;
                break;
            }
            if(LengthSqr(TestLine->End - InteractingWith->Begin) < DistanceToSnapSqr)
            {
                InteractingWith->Begin = TestLine->End;
                break;
            }
            if(LengthSqr(TestLine->Begin - InteractingWith->Begin) < DistanceToSnapSqr)
            {
                InteractingWith->Begin = TestLine->Begin;
                break;
            }
        }
    }
    
}
#endif

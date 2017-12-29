/*
New File
*/

inline void PushBitmap(particle_system_settings *Settings, bitmap *Bitmap) {
    Assert(Settings->BitmapCount < ArrayCount(Settings->Bitmaps));
    Settings->Bitmaps[Settings->BitmapCount++] = Bitmap;
    
}

internal inline particle_system_settings InitParticlesSettings() {
    particle_system_settings Set = {};
    Set.MaxLifeSpan = Set.LifeSpan = 3.0f;
    Set.VelBias = {};
    return Set;
}

internal inline void InitParticleSystem(particle_system *System, particle_system_settings *Set, u32 MaxParticleCount = DEFAULT_MAX_PARTICLE_COUNT) {
    *System = {};
    System->Entropy = Seed(0);
    System->Set = *Set;
    System->MaxParticleCount = DEFAULT_MAX_PARTICLE_COUNT;
    System->Active = true;
}

inline void Reactivate(particle_system *System) {
    System->Set.LifeSpan = System->Set.MaxLifeSpan;
    System->Active = true;
}

internal inline void DrawParticleSystem(particle_system *System, render_group *RenderGroup, r32 dt, v2 Origin, v3 Acceleration, v2 CamPos) {
    if(System->Active) {
        real32 GridScale = 0.2f;
        real32 Inv_GridScale = 1.0f / GridScale;
        
        v3 CelGridOrigin = ToV3(Origin, 0);
        
        for(u32 ParticleIndex = 0;
            ParticleIndex < 1;
            ++ParticleIndex)
        {
            particle *Particle = System->Particles + System->NextParticle++;
            Particle->Color = V4(1, 1, 1, 1.0f);
            
            //NOTE(oliver): Paricles start with motion 
            
            Particle->P = V3(RandomBetween(&System->Entropy, -0.01f, 0.01f),
                             0,
                             0);
            Particle->dP = V3(RandomBetween(&System->Entropy, System->Set.VelBias.Min.X, System->Set.VelBias.Min.X),
                              RandomBetween(&System->Entropy, System->Set.VelBias.Min.Y, System->Set.VelBias.Max.Y),
                              0);
            Particle->ddP = Acceleration;
            
            
            Particle->dColor = 0.002f*V4(1, 1, 1, 1);
            if(System->NextParticle == System->MaxParticleCount)
            {
                System->NextParticle = 0;
            }
        }
        
        ZeroStruct(System->ParticleGrid);
        
        for(u32 ParticleIndex = 0;
            ParticleIndex < System->MaxParticleCount;
            ++ParticleIndex)
        {
            particle *Particle = System->Particles + ParticleIndex;
            
            v3 P = Inv_GridScale*(Particle->P - CelGridOrigin);
            
            s32 CelX = TruncateReal32ToInt32(P.X);
            s32 CelY = TruncateReal32ToInt32(P.Y);
            
            if(CelX >= CEL_GRID_SIZE){ CelX = CEL_GRID_SIZE - 1;}
            if(CelY >= CEL_GRID_SIZE){ CelY = CEL_GRID_SIZE - 1;}
            
            particle_cel *Cel = &System->ParticleGrid[CelY][CelX];
            
            r32 ParticleDensity = Particle->Color.A;
            Cel->Density += ParticleDensity;
            Cel->dP += ParticleDensity*Particle->dP;
        }
        
#if Particles_DrawGrid
        {
            //NOTE(oliver): To draw Eulerian grid
            
            for(u32 Y = 0;
                Y < CEL_GRID_SIZE;
                ++Y)
            {
                for(u32 X = 0;
                    X < CEL_GRID_SIZE;
                    ++X)
                {
                    particle_cel *Cel = &System->ParticleGrid[Y][X];
                    
                    v3 P = CelGridOrigin + V3(GridScale*(r32)X, GridScale*(r32)Y, 0);
                    
                    real32 Density = 0.1f*Cel->Density;
                    v4 Color = V4(Density, Density, Density, 1);
                    rect2 Rect = Rect2MinDim(P.XY, V2(GridScale, GridScale));
                    PushRectOutline(RenderGroup, Rect, 1,Color);
                }
            }
        }
#endif
        for(u32 ParticleIndex = 0;
            ParticleIndex < System->MaxParticleCount;
            ++ParticleIndex)
        {
            particle *Particle = System->Particles + ParticleIndex;
            
            v3 P = Inv_GridScale*(Particle->P - CelGridOrigin);
            
            s32 CelX = TruncateReal32ToInt32(P.X);
            s32 CelY = TruncateReal32ToInt32(P.Y);
            
            if(CelX >= CEL_GRID_SIZE - 1){ CelX = CEL_GRID_SIZE - 2;}
            if(CelY >= CEL_GRID_SIZE - 1){ CelY = CEL_GRID_SIZE - 2;}
            
            if(CelX < 1){ CelX = 1;}
            if(CelY < 1){ CelY = 1;}
            
            particle_cel *CelCenter = &System->ParticleGrid[CelY][CelX];
            particle_cel *CelLeft = &System->ParticleGrid[CelY][CelX - 1];
            particle_cel *CelRight = &System->ParticleGrid[CelY][CelX + 1];
            particle_cel *CelUp = &System->ParticleGrid[CelY + 1][CelX];
            particle_cel *CelDown = &System->ParticleGrid[CelY - 1][CelX];
            
            
            v3 VacumDisplacement = Acceleration;//V3(0, 0, 0);
            r32 DisplacmentCoeff = 0.6f;
            
            VacumDisplacement += (CelCenter->Density - CelRight->Density)*V3(1, 0, 0);
            VacumDisplacement += (CelCenter->Density - CelLeft->Density)*V3(-1, 0, 0);
            VacumDisplacement += (CelCenter->Density - CelUp->Density)*V3(0, 1, 0);
            VacumDisplacement += (CelCenter->Density - CelDown->Density)*V3(0, -1, 0);
            
            v3 ddP = Particle->ddP + DisplacmentCoeff*VacumDisplacement;
            
            //NOTE(oliver): Move particle
            Particle->P = 0.5f*(Square(dt)*ddP) + dt*Particle->dP + Particle->P;
            Particle->dP = dt*ddP + Particle->dP;
            
            //NOTE(oliver): Collision with ground
            if(Particle->P.Y < 0.0f)
            {
                r32 CoefficentOfResitution = 0.5f;
                Particle->P.Y = 0.0f;
                Particle->dP.Y = -Particle->dP.Y*CoefficentOfResitution;
                Particle->dP.X *= 0.8f;
            }
            
            //NOTE(oliver): Color update
            Particle->Color += dt*Particle->dColor;
            v4 Color = Particle->Color;
            
            real32 FadeThreshold = 1.0f;//0.75f;
            if(Particle->Color.A > FadeThreshold)
            {
                Color.A = FadeThreshold*MapRange_And_Clamp01(1.0f, Particle->Color.A, FadeThreshold);
            }
            particle_system_settings *Set = &System->Set;
            
            bitmap *Bitmap = Set->Bitmaps[Set->BitmapIndex++];
            
            if(Set->BitmapIndex >= Set->BitmapCount) {
                Set->BitmapIndex = 0;
            }
            
            
            v3 Pos = Particle->P + ToV3(Origin, 1) - ToV3(CamPos, 0);
            PushBitmap(RenderGroup, Pos, Bitmap, 0.8f, Rect2(0, 0, RenderGroup->ScreenDim.X, RenderGroup->ScreenDim.Y), V4(1, 1, 1, 1));
        }
        System->Set.LifeSpan -= dt;
        if(System->Set.LifeSpan <= 0.0f) {
            if(System->Set.Loop) {
                Reactivate(System);
            } else {
                System->Active = false;
            }
            
        }
    }
}
/*
New File
*/

#define Particles_DrawGrid 0
struct particle
{
    v3 ddP;
    v3 dP;
    v3 P;
    v4 dColor;
    v4 Color;
    
};

struct particle_cel
{
    real32 Density;
    v3 dP;
};

struct particle_system_settings {
    r32 LifeSpan;
    r32 MaxLifeSpan;
    b32 Loop;
    
    u32 BitmapCount;
    bitmap *Bitmaps[32];
    u32 BitmapIndex;
    rect2 VelBias;
};

#define CEL_GRID_SIZE 32
#define DEFAULT_MAX_PARTICLE_COUNT 32

struct particle_system {
    
    particle_cel ParticleGrid[CEL_GRID_SIZE][CEL_GRID_SIZE];
    u32 NextParticle;
    u32 MaxParticleCount;
    particle Particles[DEFAULT_MAX_PARTICLE_COUNT];
    particle_system_settings Set;
    
    random_series Entropy;
    b32 Active;
};



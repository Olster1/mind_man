#define MemoryBarrier() { SDL_atomic_t A = {}; SDL_AtomicAdd(&A, 1); }
#define _ReadWriteBarrier() { MemoryBarrier(); SDL_CompilerBarrier(); }

struct PlatformSoundOutput{
    int32 sampleRate;
    int32 RunningSampleIndex;
    uint8 BytesPerSample;
    uint32 SizeOfBuffer;
    u32 BytesPerFrame;
    uint32 SamplesPerFrame;
    uint32 SafetyBytes;
};
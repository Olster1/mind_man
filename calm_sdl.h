#if MAC_PORT
#define MemoryBarrier()  __sync_synchronize()
#define _ReadWriteBarrier() { SDL_CompilerBarrier(); }
#endif

struct PlatformSoundOutput{
    int32 sampleRate;
    int32 RunningSampleIndex;
    uint8 BytesPerSample;
    uint32 SizeOfBuffer;
    u32 BytesPerFrame;
    uint32 SamplesPerFrame;
    uint32 SafetyBytes;
};
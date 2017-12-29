    struct loaded_sound {
    u32 SampleCount;
    u32 NumberOfChannels;
    s16 *Data;
};

struct playing_sound
{
    loaded_sound *Sound;
    u32 SampleIndex;
    b32 Loop;
    r32 Volume;
    
    playing_sound *Next;
};

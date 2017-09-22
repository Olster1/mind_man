#if !defined(HANDMADE_AUDIO_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */
struct loaded_sound {
    u32 SampleCount;
    u32 NumberOfChannels;
    s16 *Data;
};

struct playing_sound
{
    //NOTE(Oliver): This has to be an ID since we could fail the initial load, and since sound cues
    // are no recurrent, unlike bitmaps which happen every frame, the load needs to happen away from the
    // intial push, unlike out push bitmap call. 
    sound_ID Sound;
    real32 CursorPos;
    v2 CurrentVolumes;
    v2 TargetVolumes;
    v2 dCurrentVolumes;
    real32 dSample;
    
    playing_sound *NextSound;
    
};


struct audio_state
{
    playing_sound *FirstPlayingSound;
    playing_sound *FreePlayingSound;
    
    memory_arena *PermArena;
};


#define HANDMADE_AUDIO_H
#endif

/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#define CharToUInt(a, b, c, d) (((uint32)(a) << 0) |  ((uint32)(b) << 8) |((uint32)(c) << 16) | ((uint32)(d) << 24))

enum wav_ID
{
    RIFF = CharToUInt('R', 'I', 'F', 'F'),
    WAVE = CharToUInt('W', 'A', 'V', 'E'),
    FMT = CharToUInt('f', 'm', 't', ' '),
    DATA = CharToUInt('d', 'a', 't', 'a'),
    
};

#pragma pack(push, 1)
struct RIFF_header
{
    uint32 RIFF;
    uint32 FileLength;
    uint32 WAVE;
};

struct chunk_header
{
    uint32 ID;
    uint32 Size;
};

struct wav_format_chunk
{
    uint16 FormatTag;
    uint16 nChannels;
    uint32 nSamplesPerSec;
    uint32 nAvgBytesPerSec;
    uint16 nBlockAlign;
    uint16 wBitsPerSample;
    uint16 cbSize;
    uint16 ValidBitsPerSample;
    uint32 dwChannelMask;
    uint8 SubFormat[16];
    
};

#pragma pack(pop)


struct iterator
{
    chunk_header *CurrentChunk;
    uint8 *At;
    uint8 *StopAt;
    
};

inline iterator
BeginAt(void *StartAt, uint32 FileSize)
{
    iterator Result = {};
    
    Result.CurrentChunk = (chunk_header *)StartAt;
    Result.At = (uint8 *)StartAt;
    Result.StopAt = Result.At + FileSize;
    
    
    return Result;
}

inline bool32
IsValid(iterator *Iter)
{
    bool32 Result = (Iter->At < Iter->StopAt);
    
    return Result;
}

inline void
NextChunk(iterator *Iter)
{
    uint32 Size = (Iter->CurrentChunk->Size + 1) & ~1;
    Iter->At += Size + sizeof(chunk_header);
    Iter->CurrentChunk = (chunk_header *)(Iter->At);
    
}

inline void *
GetChunkData(iterator *Iter)
{
    void *Result = (void *)(Iter->CurrentChunk + 1);
    return Result;
}

internal loaded_sound
LoadWavFileDEBUG(game_memory *Memory, char *FileName, u32 StartSampleIndex, u32 EndSampleIndex, memory_arena *Arena)
{
    loaded_sound Result = {};
    
    //sound_asset *MetaData = GetSound(Assets, FileName);
    
    size_t FileSize = Memory->PlatformFileSize(FileName);
    void *AllocatedMemory = PushSize(Arena, FileSize);
    
    game_file_handle FileHandle = Memory->PlatformBeginFile(FileName);
    
    file_read_result ReadResult = Memory->PlatformReadFile(FileHandle, AllocatedMemory, FileSize, 0);
    
    Memory->PlatformEndFile(FileHandle);
    
    if(ReadResult.Data)
    {
        
        RIFF_header *Header = (RIFF_header *)ReadResult.Data;
        
        Assert(Header->RIFF == RIFF);        
        Assert(Header->WAVE == WAVE);
        
        int16 *SampleData = 0;
        uint16 NumberOfChannels = 0;
        uint32 TotalSizeOfSamples = 0;
        
        for(iterator Iter = BeginAt((Header + 1), Header->FileLength - 4);
            IsValid(&Iter);
            NextChunk(&Iter))
        {
            switch(Iter.CurrentChunk->ID)
            {
                case FMT:
                {
                    Assert(Iter.CurrentChunk->ID == FMT);
                    
                    wav_format_chunk *Format = (wav_format_chunk *)GetChunkData(&Iter);
                    Assert(Format->wBitsPerSample == 16);
                    Assert(Format->FormatTag == 1);
                    Assert(Format->nSamplesPerSec == 48000);
                    Assert(Format->nChannels == 1 || Format->nChannels == 2);  
                    
                    NumberOfChannels = Format->nChannels;
                    
                    
                } break;
                
                case DATA:
                {
                    Assert(Iter.CurrentChunk->ID == DATA);
                    TotalSizeOfSamples = Iter.CurrentChunk->Size;
                    
                    SampleData = (int16 *)GetChunkData(&Iter);
                    
                } break;
                
                default :
                {
                    
                }
            }
            
        }
        
        
        Result.SampleCount = TotalSizeOfSamples / (NumberOfChannels*sizeof(int16));
        Result.NumberOfChannels = NumberOfChannels;
        
        
        if(Result.NumberOfChannels == 1)
        {
            Result.Samples[0] = (int16 *)SampleData;
            Result.Samples[1] = (int16 *)SampleData;
            
        }
        else if(Result.NumberOfChannels == 2)
        {
            
            for(u32 SampleIndex = 0;
                SampleIndex < Result.SampleCount;
                ++SampleIndex)
            {
                SampleData[SampleIndex] = SampleData[2*SampleIndex];
            }
            
            Result.Samples[0] = (int16 *)SampleData;
            Result.Samples[1] = (int16 *)SampleData;
        }
        else
        {
            Assert(!"Number of channels not supported!");
        }
        
        
        if(EndSampleIndex != 0)
        {
            Assert(EndSampleIndex <= Result.SampleCount);
            Result.SampleCount = EndSampleIndex - StartSampleIndex;
            
            Result.Samples[0] = (int16 *)(SampleData + StartSampleIndex);
            Result.Samples[1] = (int16 *)(SampleData + StartSampleIndex);
            
        }
        else
        {
            u32 AlignedSampleCount = Align4(Result.SampleCount);
            
            for(u32 SampleIndex = Result.SampleCount;
                SampleIndex < AlignedSampleCount;
                ++SampleIndex)
            {
                SampleData[SampleIndex] = 0;
            }
            
            Result.SampleCount = AlignedSampleCount;
        }
    }
    
    return Result;
    
}


internal void
InitializeAudioState(audio_state *AudioState, memory_arena *Arena)
{
    AudioState->FirstPlayingSound = 0;
    AudioState->FreePlayingSound = 0;
    
    AudioState->PermArena = Arena;
}



internal void
MixPlayingSounds(game_output_sound_buffer *SoundBuffer, audio_state *AudioState,
                 memory_arena *TempArena)
{
    
    __m128i *StartAddress = (__m128i *)SoundBuffer->Samples;
    temp_memory SoundBufferMem = MarkMemory(TempArena);
    
    Assert((SoundBuffer->SamplesToWrite & 7) == 0);
    
    real32 Inv_SamplesPerSecond = 1.0f / SoundBuffer->SampleRate;
    
    uint32 Size = (SoundBuffer->SamplesToWrite * sizeof(int16));
    Size = (Size + 15) & ~15;
    __m128 *Samples0 = (__m128 *)PushArray(TempArena, real32, Size, 16);
    __m128 *Samples1 = (__m128 *)PushArray(TempArena, real32, Size, 16);
    
    __m128 Zero = _mm_set1_ps(0.0f);
    
    uint32 Size_4 = Size / 4;
    
    for(uint32 SampleIndex = 0;
        SampleIndex < Size_4;
        ++SampleIndex)
    {
        Samples0[SampleIndex] = Zero;
        Samples1[SampleIndex] = Zero;
        
    }
    
    __m128 One = _mm_set1_ps(1.0f);
    
    for(playing_sound **PlayingSoundPtr = &AudioState->FirstPlayingSound;
        *PlayingSoundPtr;
        )
    {
        
        playing_sound *PlayingSound = *PlayingSoundPtr;
        bool32 SoundFinished = false;
        
        __m128 *Dest0 = (__m128 *)Samples0;
        __m128 *Dest1 = (__m128 *)Samples1;
        
        u32 TotalSamplesLeftToWrite = SoundBuffer->SamplesToWrite;
        
        while(SoundFinished == false && TotalSamplesLeftToWrite > 0)
        {
            loaded_sound *Sound = PlayingSound->Sound;
            
            if(Sound)
            {
#if 0
                openhha_sound_info *SoundInfo = GetSoundInfo(Assets, PlayingSound->ID);
                
                sound_ID NextSoundID = GetNextSoundID(PlayingSound->ID, SoundInfo);
                
                PrefetchSound(Assets, NextSoundID);
#endif
                v2 dVolumePerSample = Inv_SamplesPerSecond*PlayingSound->dCurrentVolumes;
                
                Assert(PlayingSound->CursorPos >= 0.0f && PlayingSound->CursorPos <= Sound->SampleCount);
                Assert(PlayingSound->dSample >= 0.0f);
                
                
                uint32 SamplesToWrite = TotalSamplesLeftToWrite;
                real32 SamplesLeftInSoundFloat = ((real32)Sound->SampleCount - PlayingSound->CursorPos) / PlayingSound->dSample;
                uint32 SamplesLeftInSound = Align4(CeilRealToInt32(SamplesLeftInSoundFloat));
                
                if(SamplesLeftInSound < SamplesToWrite)
                {
                    SamplesToWrite = (SamplesLeftInSound);
                }
                
                
                bool32 VolumeChangeEnded[2] = {};
                uint32 ShortestVolumeIndex = 10000;
                for(u32 ChannelIndex = 0;
                    ChannelIndex < ArrayCount(VolumeChangeEnded);
                    ++ChannelIndex)
                {
                    if(PlayingSound->dCurrentVolumes.E[ChannelIndex] != 0.0f)
                    {
                        real32 SecsUntilEnd = (PlayingSound->TargetVolumes.E[ChannelIndex] - PlayingSound->CurrentVolumes.E[ChannelIndex]);
                        uint32 SamplesUntilVolumeChangeEnds = (uint32)(SecsUntilEnd / dVolumePerSample.E[ChannelIndex]);
                        
                        if(SamplesUntilVolumeChangeEnds < SamplesToWrite)
                        {
                            SamplesToWrite = SamplesUntilVolumeChangeEnds;
                            ShortestVolumeIndex = ChannelIndex;
                        }
                    }
                }
                
                if(ShortestVolumeIndex != 10000)
                {
                    VolumeChangeEnded[ShortestVolumeIndex] = true;
                }
                
                
                //#define Mi(Value, Index) (((uint32 *)&Value)[Index])
                
                v2 Volumes = PlayingSound->CurrentVolumes;
                
                __m128 dCurrentVolume0 = _mm_set1_ps(dVolumePerSample.E[0]);
                __m128 dCurrentVolume1 = _mm_set1_ps(dVolumePerSample.E[1]);
                
                __m128 BeginVolume0 = _mm_set_ps(Volumes.E[0] + 3.0f*dVolumePerSample.E[0],
                                                 Volumes.E[0] + 2.0f*dVolumePerSample.E[0],
                                                 Volumes.E[0] + 1.0f*dVolumePerSample.E[0],
                                                 Volumes.E[0]);
                
                __m128 BeginVolume1 = _mm_set_ps(Volumes.E[1] + 3.0f*dVolumePerSample.E[1],
                                                 Volumes.E[1] + 2.0f*dVolumePerSample.E[1],
                                                 Volumes.E[1] + 1.0f*dVolumePerSample.E[1],
                                                 Volumes.E[1]);
                
                __m128 SamplesToWrite_Wide = _mm_set1_ps((real32)SamplesToWrite);
                __m128 InvSamplesToWrite = _mm_set1_ps((1.0f / (real32)SamplesToWrite));
                
                __m128 EndVolume0 = _mm_add_ps(BeginVolume0, _mm_mul_ps(SamplesToWrite_Wide, dCurrentVolume0));
                __m128 EndVolume1 = _mm_add_ps(BeginVolume1, _mm_mul_ps(SamplesToWrite_Wide, dCurrentVolume1));
                
                __m128 VolumeC_0 = _mm_mul_ps(_mm_sub_ps(EndVolume0, BeginVolume0), InvSamplesToWrite);
                __m128 VolumeC_1 = _mm_mul_ps(_mm_sub_ps(EndVolume1, BeginVolume1), InvSamplesToWrite);
                
                v2 EndVolume = PlayingSound->CurrentVolumes + (real32)SamplesToWrite*dVolumePerSample;
                
                real32 BeginSampleCount = PlayingSound->CursorPos;
                real32 EndSampleCount = BeginSampleCount + (real32)SamplesToWrite*PlayingSound->dSample;
                real32 SampleC = (EndSampleCount - BeginSampleCount) / (real32)SamplesToWrite;
                
                for(real32 LoopIndex = 0;
                    LoopIndex < SamplesToWrite;
                    LoopIndex += 4)
                {
                    real32 SampleCount = BeginSampleCount + SampleC*(real32)LoopIndex;
                    
                    __m128 LoopIndex4 = _mm_set_ps((real32)LoopIndex + 3,
                                                   (real32)LoopIndex + 2,
                                                   (real32)LoopIndex + 1,
                                                   (real32)LoopIndex);
                    
                    __m128 Volume0 = _mm_add_ps(BeginVolume0, _mm_mul_ps(VolumeC_0, LoopIndex4));
                    __m128 Volume1 = _mm_add_ps(BeginVolume1, _mm_mul_ps(VolumeC_1, LoopIndex4));
                    
                    __m128 Pos = _mm_set_ps(SampleCount + 3*PlayingSound->dSample,
                                            SampleCount + 2*PlayingSound->dSample,
                                            SampleCount + 1*PlayingSound->dSample,
                                            SampleCount);
                    
                    __m128i SampleBlendIndex = _mm_cvttps_epi32(Pos);
                    __m128 tBlend = _mm_sub_ps(Pos, _mm_cvtepi32_ps(SampleBlendIndex));
                    
                    int16 *SampleA0 = &Sound->Samples[0][Mi(SampleBlendIndex, 0)];
                    int16 *SampleA1 = &Sound->Samples[0][Mi(SampleBlendIndex, 1)];
                    int16 *SampleA2 = &Sound->Samples[0][Mi(SampleBlendIndex, 2)];
                    int16 *SampleA3 = &Sound->Samples[0][Mi(SampleBlendIndex, 3)];
                    
                    int16 *SampleB0 = &Sound->Samples[1][Mi(SampleBlendIndex, 0)];
                    int16 *SampleB1 = &Sound->Samples[1][Mi(SampleBlendIndex, 1)];
                    int16 *SampleB2 = &Sound->Samples[1][Mi(SampleBlendIndex, 2)];
                    int16 *SampleB3 = &Sound->Samples[1][Mi(SampleBlendIndex, 3)];
                    
                    __m128 Dest0_1 = _mm_load_ps((float *)Dest0);
                    __m128 Dest1_1 = _mm_load_ps((float *)Dest1);
                    
                    __m128 A0 = _mm_set_ps((real32)*SampleA3,
                                           (real32)*SampleA2,
                                           (real32)*SampleA1,
                                           (real32)*SampleA0);
                    
                    __m128 B0 = _mm_set_ps((real32)*(SampleA3 + 1),
                                           (real32)*(SampleA2 + 1),
                                           (real32)*(SampleA1 + 1),
                                           (real32)*(SampleA0 + 1));
                    
                    __m128 A1 = _mm_set_ps((real32)*SampleB3,
                                           (real32)*SampleB2,
                                           (real32)*SampleB1,
                                           (real32)*SampleB0);
                    
                    __m128 B1 = _mm_set_ps((real32)*(SampleB3 + 1),
                                           (real32)*(SampleB2 + 1),
                                           (real32)*(SampleB1 + 1),
                                           (real32)*(SampleB0 + 1));
                    
                    __m128 Inv_tBlend = _mm_sub_ps(One, tBlend);
                    
                    __m128 Value0 = _mm_add_ps(_mm_mul_ps(Inv_tBlend, A0), _mm_mul_ps(tBlend, B0));
                    __m128 Value1 = _mm_add_ps(_mm_mul_ps(Inv_tBlend, A1), _mm_mul_ps(tBlend, B1));
                    
                    Dest0_1 = _mm_add_ps(Dest0_1, _mm_mul_ps(Value0, Volume0));
                    Dest1_1 = _mm_add_ps(Dest1_1, _mm_mul_ps(Value1, Volume1));
                    
                    _mm_store_ps((float *)(Dest0++), Dest0_1);
                    _mm_store_ps((float *)(Dest1++), Dest1_1);
                    
                }
                
                PlayingSound->CurrentVolumes = EndVolume;
                
                PlayingSound->CursorPos = EndSampleCount;
                TotalSamplesLeftToWrite -= SamplesToWrite;
                
                for(u32 ChannelIndex = 0;
                    ChannelIndex < Sound->NumberOfChannels;
                    ++ChannelIndex)
                {
                    if(VolumeChangeEnded[ChannelIndex])
                    {
                        PlayingSound->CurrentVolumes.E[ChannelIndex] = PlayingSound->TargetVolumes.E[ChannelIndex];
                        PlayingSound->dCurrentVolumes = V2(0.0f, 0.0f);
                    }
                }
                
                
                if(SamplesLeftInSound == SamplesToWrite)
                {
                    SoundFinished = true;
                    PlayingSound->CursorPos = 0;
                }
                Assert(PlayingSound->CursorPos >= 0.0f && PlayingSound->CursorPos <= Sound->SampleCount);
                
                
            }
        }
        
        if(SoundFinished && !PlayingSound->Loop)
        {
            playing_sound *Next = PlayingSound->NextSound;
            
            PlayingSound->NextSound = AudioState->FreePlayingSound;
            AudioState->FreePlayingSound = PlayingSound;
            
            *PlayingSoundPtr = Next;
        }
        
        else
        {
            PlayingSoundPtr = &PlayingSound->NextSound;
        }
        
    }
    
    
    //Assert((SoundBuffer->SamplesToWrite & 15) == 0);
    
    for(int32 SampleIndex = 0;
        SampleIndex < SoundBuffer->SamplesToWrite;
        SampleIndex += 4)
    {
        
        __m128i Value0 = _mm_cvtps_epi32(_mm_load_ps((float *)(Samples0++)));
        __m128i Value1 = _mm_cvtps_epi32(_mm_load_ps((float *)(Samples1++)));
        
#if 0
        Value0 = _mm_set1_epi32(0xCCCCCCCC);
        Value1 = _mm_set1_epi32(0xffffffff);
#endif        
        
        __m128i L0R0 = _mm_unpacklo_epi32(Value0, Value1);
        __m128i L1R1 = _mm_unpackhi_epi32(Value0, Value1);
        __m128i Packed = _mm_packs_epi32(L0R0, L1R1);
        
        
        _mm_store_si128((__m128i *)StartAddress++, Packed);
        
    }
    ReleaseMemory(&SoundBufferMem);
    
}

//Can use this to add a sound to that was already playing
inline void PlaySound(audio_state *AudioState, playing_sound *PlayingSound) {
    PlayingSound->NextSound = AudioState->FirstPlayingSound;
    AudioState->FirstPlayingSound = PlayingSound;
    
}

internal playing_sound *
PlaySound(audio_state *AudioState, loaded_sound *Sound)
{
    if(!AudioState->FreePlayingSound)
    {
        AudioState->FreePlayingSound = PushStruct(AudioState->PermArena, playing_sound);
        AudioState->FreePlayingSound->NextSound = 0;
    }
    
    playing_sound *PlayingSound = AudioState->FreePlayingSound;
    AudioState->FreePlayingSound = PlayingSound->NextSound;
    
    PlayingSound->Sound = Sound;
    PlayingSound->CursorPos = 0;
    
    PlayingSound->CurrentVolumes = V2(1, 1);
    PlayingSound->TargetVolumes = V2(1, 1);
    PlayingSound->dCurrentVolumes = V2(0, 0);
    PlayingSound->dSample = 1.0f;
    PlayingSound->Loop = false;
    
    PlaySound(AudioState, PlayingSound);
    
    return PlayingSound;
}

//These are for the below stop and pause sound methods
enum sound_action {
    SOUND_STOP, 
    SOUND_PAUSE,
};
//

inline void SearchSound(audio_state *AudioState, playing_sound *Sound, sound_action Action) {
    
    for(playing_sound **PlayingSoundPtr = &AudioState->FirstPlayingSound; *PlayingSoundPtr; 
        ) {
        playing_sound *PlayingSound = *PlayingSoundPtr;
        
        if(PlayingSound == Sound) {
            switch(Action) {
                case SOUND_PAUSE: {
                    playing_sound *Next = PlayingSound->NextSound;
                    *PlayingSoundPtr = Next;
                } break;
                case SOUND_STOP: {
                    playing_sound *Next = PlayingSound->NextSound;
                    
                    PlayingSound->NextSound = AudioState->FreePlayingSound;
                    AudioState->FreePlayingSound = PlayingSound;
                    
                    *PlayingSoundPtr = Next;
                } break;
            }
            
            break;
        }
        PlayingSoundPtr = &PlayingSound->NextSound;
    }
    
}

//This assumes to keep the sound and are in charge of freeing the memory when finished with. Either allowing the sound to end or useing the stop sound method
inline void PauseSound(audio_state *AudioState, playing_sound *Sound) {
    SearchSound(AudioState, Sound, SOUND_PAUSE);
}

inline void StopSound(audio_state *AudioState, playing_sound *Sound) {
    SearchSound(AudioState, Sound, SOUND_STOP);
}

internal void
ChangeSoundVolume(playing_sound *PlayingSound, v2 EndVolume, real32 RampInSecs)
{
    if(RampInSecs <= 0.0f)
    {
        PlayingSound->CurrentVolumes = PlayingSound->TargetVolumes = EndVolume;
    }
    else
    {
        PlayingSound->TargetVolumes = EndVolume;
        PlayingSound->dCurrentVolumes = (1.0f / RampInSecs)*(EndVolume - PlayingSound->CurrentVolumes);
    }
    
}

inline void
ChangeSoundPitch(playing_sound *PlayingSound, real32 PitchPercent)
{
    if(PitchPercent < 0.0f)
    {
        PitchPercent = 0.0f;
    }
    PlayingSound->dSample = PitchPercent;
    
}

internal void
GameOutputSound(game_state *GameState, game_output_sound_buffer *soundBuffer){
    
#if 0
    int32 toneHz = 256;
    int32 toneVolume = 6000;
    int32 samplesPerCycle = soundBuffer->sampleRate / toneHz;
#endif
    
    int16 *startAddress = soundBuffer->Samples;
    for(int32 sampleIndex = 0;
        sampleIndex < soundBuffer->SamplesToWrite;
        sampleIndex++){
        int16 sampleValue = 0;
#if 0
        int16 sampleValue = (int16)(sinf(GameState->tSine) * (real32)toneVolume);
#endif
        *startAddress++ = sampleValue;
        *startAddress++ = sampleValue;
#if 0
        real32 anglePerFrame = (1.0f / (real32)samplesPerCycle) * (2.0f * (real32)PI32);
        GameState->tSine += anglePerFrame;
        
        if(GameState->tSine > 2 * PI32)
        {
            GameState->tSine -= (real32)(2.0f * PI32);
        }
#endif
    }
    
}


internal void GameGetSoundSamples(game_memory *Memory, game_output_sound_buffer *SoundBuffer)
{
    
    game_state *GameState = (game_state *)Memory->GameStorage;
    
    MixPlayingSounds(SoundBuffer, &GameState->AudioState,
                     &GameState->TransientArena);
}
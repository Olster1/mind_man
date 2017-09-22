/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

internal void
InitializeAudioState(audio_state *AudioState, memory_arena *Arena)
{
    AudioState->FirstPlayingSound = 0;
    AudioState->FreePlayingSound = 0;
    
    AudioState->PermArena = Arena;
}

inline sound_ID
GetNextSoundID(sound_ID SoundID, hha_sound_info *SoundInfo)
{
    sound_ID Result = {};
    switch(SoundInfo->SoundChain)
    {
        case SoundChain_None:
        {
            //NOTE(oliver): Nothing to do.
        } break;
        
        case SoundChain_Repeat:
        {
            Result = SoundID;
        } break;
        
        case SoundChain_Advance:
        {
            Result = {SoundID.Value + 1};            
        } break;
        
        default:
        {
            InvalidCodePath;
        }
        
    }
    
    return Result;
}


internal void
MixPlayingSounds(game_sound_buffer *SoundBuffer, audio_state *AudioState,
                 memory_arena *TempArena)
{
    
    __m128i *StartAddress = (__m128i *)SoundBuffer->Samples;
    temporary_memory SoundBufferMem = BeginTemporaryMemory(TempArena);
    
    Assert((SoundBuffer->SamplesToWrite & 7) == 0);
    
    real32 Inv_SamplesPerSecond = 1.0f / SoundBuffer->SampleRate;
    
    uint32 Size = (SoundBuffer->SamplesToWrite * sizeof(int16));
    Size = (Size + 15) & ~15;
    __m128 *Samples0 = (__m128 *)PushArray(TempArena, Size, real32, 16);
    __m128 *Samples1 = (__m128 *)PushArray(TempArena, Size, real32, 16);
    
    __m128 Zero = _mm_set1_ps(0.0f);
    
    uint32 Size_4 = Size / 4;
    
    u32 GenerationID = BeginGeneration(Assets); 
    
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
                hha_sound_info *SoundInfo = GetSoundInfo(Assets, PlayingSound->ID);
                
                sound_ID NextSoundID = GetNextSoundID(PlayingSound->ID, SoundInfo);
                
                PrefetchSound(Assets, NextSoundID);
                
                v2 dVolumePerSample = PlayingSound->dCurrentVolumes * Inv_SamplesPerSecond;
                
                Assert(PlayingSound->CursorPos >= 0.0f && PlayingSound->CursorPos <= SoundInfo->SampleCount);
                Assert(PlayingSound->dSample >= 0.0f);
                
                
                uint32 SamplesToWrite = TotalSamplesLeftToWrite;
                real32 SamplesLeftInSoundFloat = ((real32)SoundInfo->SampleCount - PlayingSound->CursorPos) / PlayingSound->dSample;
                uint32 SamplesLeftInSound = Align4(CeilReal32ToInt32(SamplesLeftInSoundFloat));
                
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
                
                
#define Mi(Value, Index) (((uint32 *)&Value)[Index])
                
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
                    ChannelIndex < SoundInfo->NumberOfChannels;
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
                    if(NextSoundID.Value)
                    {
                        Assert(PlayingSound->CursorPos >= (real32)(SoundInfo->SampleCount));
                        
                        PlayingSound->ID = NextSoundID;
                        PlayingSound->CursorPos -= (real32)SoundInfo->SampleCount;
                        if(PlayingSound->CursorPos < 0)
                        {
                            PlayingSound->CursorPos = 0;
                        }
                    }
                    else
                    {
                        SoundFinished = true;
                        PlayingSound->CursorPos = 0;
                    }
                }
                Assert(PlayingSound->CursorPos >= 0.0f && PlayingSound->CursorPos <= SoundInfo->SampleCount);
                
                
            }
            else
            {
                LoadSound(Assets, PlayingSound->ID);
                break;
            }
        }
        
        if(SoundFinished)
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
    EndTemporaryMemory(&SoundBufferMem);
    EndGeneration(Assets, GenerationID);
    
}



internal playing_sound *
PlaySound(audio_state *AudioState, sound_ID ID)
{
    TIMED_FUNCTION();
    
    if(!AudioState->FreePlayingSound)
    {
        AudioState->FreePlayingSound = PushStruct(AudioState->PermArena, playing_sound);
        AudioState->FreePlayingSound->NextSound = 0;
    }
    
    playing_sound *PlayingSound = AudioState->FreePlayingSound;
    AudioState->FreePlayingSound = PlayingSound->NextSound;
    
    PlayingSound->ID = ID;
    PlayingSound->CursorPos = 0;
    
    PlayingSound->CurrentVolumes = V2(1, 1);
    PlayingSound->TargetVolumes = V2(1, 1);
    PlayingSound->dCurrentVolumes = V2(0, 0);
    PlayingSound->dSample = 1.1f;
    
    
    
    PlayingSound->NextSound = AudioState->FirstPlayingSound;
    
    AudioState->FirstPlayingSound = PlayingSound;
    
    return PlayingSound;
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


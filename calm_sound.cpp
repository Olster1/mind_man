
internal void
GameWriteAudioSamples(game_memory *Memory, game_sound_buffer *Buffer)
{
    game_state *GameState = (game_state *)Memory->GameStorage;
    
#if 0 //NOTE(oliver): sine wave testing code
    r32 Percentage = (Memory->MouseInput[0].Y - 270) / 540.0f;
    u32 Range = 200;
    u32 NoteBase = 440;
    s32 NoteFreq = (s32)(NoteBase + Percentage*Range);    
    r32 SecondsPerSample = 1.0f / (r32)Buffer->SamplesPerSecond;
    r32 WavesPerSample = NoteFreq * SecondsPerSample;
    r32 Volume = 7000;
    
    r32 IncrementValue = WavesPerSample*Tau32;
    s16 *SampleAt = (s16 *)Buffer->Samples;
    for(u32 Index = 0;
        Index < Buffer->SamplesToWrite;
        ++Index)
    {
        r32 Value = Sin(GameState->Angle)*Volume;
        GameState->Angle += IncrementValue;
        if(GameState->Angle > Tau32)
        {
            GameState->Angle -= Tau32;
        }
        
        
        *SampleAt++ = (s16)Value;
        *SampleAt++ = (s16)Value;
    }
#else
    temp_memory TempMem = MarkMemory(&GameState->MemoryArena);
    
    u32 SampleBufferSize = Buffer->ChannelCount*Buffer->SamplesToWrite*sizeof(s16);
    r32 *SamplesBuffer = PushArray(TempMem.Arena, r32, SampleBufferSize);
    ClearMemory(SamplesBuffer, SampleBufferSize);
    
    b32 AdvancePtr = true;
    for(playing_sound **PlayingSoundPtr = &GameState->PlayingSounds;
        *PlayingSoundPtr;
        )
    {
        playing_sound *PlayingSound = *PlayingSoundPtr;
        
        r32 *SamplesAt = SamplesBuffer;
        
        Assert((PlayingSound->SampleIndex / PlayingSound->Sound->NumberOfChannels) < PlayingSound->Sound->SampleCount);
        u32 SamplesLeftInSound = PlayingSound->Sound->SampleCount - (PlayingSound->SampleIndex / PlayingSound->Sound->NumberOfChannels);
        u32 SampleCount = Buffer->SamplesToWrite;
        if(SampleCount > SamplesLeftInSound)
        {
            SampleCount = SamplesLeftInSound;
        }
        
        r32 Volume = PlayingSound->Volume;
        u32 SampleIndex = 0;
        if(Memory->SoundOn) {
            for(;
                SampleIndex < SampleCount;
                ++SampleIndex)
            {
                //TODO(ollie): Handle single channel data
                *SamplesAt++ += Volume*PlayingSound->Sound->Data[PlayingSound->SampleIndex++];
                *SamplesAt++ += Volume*PlayingSound->Sound->Data[PlayingSound->SampleIndex++];
            }
        } else {
            // TODO(OLIVER): Is this the best way to handle no sound? 
            for(;
                SampleIndex < SampleCount;
                ++SampleIndex)
            {
                //TODO(ollie): Handle single channel data
                *SamplesAt++ += 0.0f;
                *SamplesAt++ += 0.0f; 
                PlayingSound->SampleIndex++;
                PlayingSound->SampleIndex++;
            }
        }
        
        if((PlayingSound->SampleIndex / PlayingSound->Sound->NumberOfChannels)  == PlayingSound->Sound->SampleCount)
        {
            if(PlayingSound->Loop)
            {
                PlayingSound->SampleIndex = 0;
            }
            else
            {
                *PlayingSoundPtr = PlayingSound->Next;
                PlayingSound->Next = GameState->PlayingSoundsFreeList;
                GameState->PlayingSoundsFreeList = PlayingSound;
                
                AdvancePtr = false;
            }
        }
        
        if(AdvancePtr)
        {
            PlayingSoundPtr = &(*PlayingSoundPtr)->Next;
        }
        
        AdvancePtr = true;
        
    }
    s16 *Samples = Buffer->Samples;
    
    for(u32 SampleCount = 0;
        SampleCount < Buffer->SamplesToWrite;
        ++SampleCount)
    {
        s16 LeftValue = (s16)(*SamplesBuffer++);
        s16 RightValue = (s16)(*SamplesBuffer++);
        
        *Samples++ = LeftValue;
        *Samples++ = RightValue;
    }
    
    ReleaseMemory(&TempMem);
#endif
    
}

#pragma pack(push, 1)
struct chunk_header
{
    u32 ChunkID;
    u32 ChunkSize;
};

struct wav_file_header
{
    chunk_header Header;
    u32 WaveID;
    
};

struct wave_format_chunk
{
    chunk_header Header;
    u16 FormatCode;
    u16 NumberOfChannels;
    u32 SamplesPerSecond;
    u32 BytesPerSecond;
    u16 BlockAlign;
    u16 BitsPerSample;
};

#pragma pack(pop)

#define RIFF (('R' << 0) | ('I' << 8) | ('F' << 16) | ('F' << 24))
#define FMT (('f' << 0) | ('m' << 8) | ('t' << 16) | (' ' << 24))
#define DATA (('d' << 0) | ('a' << 8) | ('t' << 16) | ('a' << 24))
#define WAVE (('W' << 0) | ('A' << 8) | ('V' << 16) | ('E' << 24))

internal loaded_sound
LoadWavFileDEBUG(game_memory *Memory, char *FileName, memory_arena *Arena)
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
        wav_file_header *Header = (wav_file_header *)ReadResult.Data;
        Assert(Header->Header.ChunkID == RIFF);
        Assert(Header->WaveID == WAVE);
        
        u8 *FileContents = (u8 *)ReadResult.Data;
        
        for(u32 Offset = sizeof(wav_file_header);
            Offset < ReadResult.Size;
            )
        {
            chunk_header *ChunkHeader = (chunk_header *)(FileContents + Offset);
            
            u32 BytesPerSample = 4;
            
            switch(ChunkHeader->ChunkID)
            {
                case FMT:
                {
                    wave_format_chunk *Format = (wave_format_chunk *)ChunkHeader;
                    
                    Assert(Format->FormatCode == WAVE_FORMAT_PCM);
                    Assert(Format->NumberOfChannels == 2);
                    Assert(Format->SamplesPerSecond == 48000);
                    Assert(Format->BlockAlign == Format->NumberOfChannels*Format->BitsPerSample / 8);
                    Assert(Format->BitsPerSample == 16);
                    
                    Assert((s32)BytesPerSample == (Format->NumberOfChannels*Format->BitsPerSample) / 8);
                    Result.NumberOfChannels = Format->NumberOfChannels;
                    
                } break;
                case DATA:
                {
                    Result.SampleCount = (ChunkHeader->ChunkSize / BytesPerSample);
                    Result.Data = (s16 *)(ChunkHeader + 1);
                } break;
                default:
                {
                    //NOTE(ollie): Not parsing the other chunks
                }
            }
            
            Offset += (ChunkHeader->ChunkSize + sizeof(chunk_header));
        }
        
    }
    else
    {
        Assert(!"Couldn't read file");
    }
    
    return Result;
}

internal void
PushSound(game_state *GameState, loaded_sound *SoundToPlay, r32 Volume = 1.0f, b32 Loop = false)
{
    playing_sound *PlayingSound = 0;
    if(GameState->PlayingSoundsFreeList)
    {
        PlayingSound = GameState->PlayingSoundsFreeList;
        GameState->PlayingSoundsFreeList = PlayingSound->Next;
    }
    else
    {
        PlayingSound = PushStruct(&GameState->MemoryArena, playing_sound);
    }
    
    PlayingSound->Sound = SoundToPlay;
    PlayingSound->SampleIndex = 0;
    PlayingSound->Loop = Loop;
    PlayingSound->Volume = Volume;
    PlayingSound->Next = GameState->PlayingSounds;
    
    GameState->PlayingSounds = PlayingSound;
    
}

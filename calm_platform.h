#if !defined(CALM_PLATFORM_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */
/*
// TODO(OLIVER): 

*/
#include <stdint.h>

//NOTE(oliver): Define types

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t memory_size;

typedef float r32;
typedef double r64;
typedef u32 b32;
typedef r32 real32;
typedef r64 real64;

typedef u8 uint8;
typedef u16 uint16;
typedef u32 uint32;
typedef u64 uint64;

typedef s8 int8;
typedef s16 int16;
typedef s32 int32;
typedef s64 int64;

typedef intptr_t intptr;

typedef b32 bool32;


struct v2
{
    union
    {
        struct
        {
            r32 X, Y;
        };
        struct
        {
            r32 U, V;
        };
    };
};

struct v2i
{
    union
    {
        struct
        {
            s32 X, Y;
        };
        struct
        {
            s32 U, V;
        };
    };
};



#define PI32 3.14159265359f
#define TAU32 6.28318530717958f

#include <float.h>
#define MAX_U32 0xFFFFFFFF
#define MAX_S32 2147483647

#define global_variable static
#define internal static

#define KiloBytes(Value) Value*1024
#define MegaBytes(Value) Value*1024*1024
#define GigaBytes(Value) Value*1024*1024*1024

#define BreakPoint int i = 0;

#define Min(A, B) (A < B) ? A : B
#define Max(A, B) (A < B) ? B : A

#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))

#define fori(Array) for(u32 Index = 0; Index < ArrayCount(Array); ++Index)
#define fori_count(Count) for(u32 Index = 0; Index < Count; ++Index)

#define forN_(Count, Name) for(u32 Name = 0; Name < Count; Name++)
#define forN_s(Count, Name) for(s32 Name = 0; Name < Count; Name++)
#define forN(Count) forN_(Count, ##Count##Index)
#define forNs(Count) forN_s(Count, ##Count##Index)



#if INTERNAL_BUILD
#define Assert(Condition) {if(!(Condition)){ u32 *ATemp_ = (u32 *)0; *ATemp_ = 0;}}
#else
#define Assert(Condition)
#endif

#define InvalidCodePath Assert(!"Invalid Code Path");

#define SOFTWARE_RENDERER 0
#define SLEEP 0
#if SOFTWARE_RENDERER
#undef SLEEP
#define SLEEP 1
#endif


#include "calm_intrinsics.h"
#include "calm_math.h"
#include "calm_print.cpp"

#define CastAs(type, value) (*((type *)&value))
#define CastVoidAs(type, value) (*((type *)value))

inline b32 IsWhiteSpace(char Char) {
    b32 Result = (Char == ' ' || Char == '\n' || Char == '\r');
    return Result;
}

struct file_read_result
{
    u32 Size;
    void *Data;
};

struct game_file_handle
{
    void *Data;
    b32 HasErrors;
};

#define PLATFORM_BEGIN_FILE(name) game_file_handle name(char *FileName)
typedef PLATFORM_BEGIN_FILE(platform_begin_file);

#define PLATFORM_BEGIN_FILE_WRITE(name) game_file_handle name(char *FileName)
typedef PLATFORM_BEGIN_FILE_WRITE(platform_begin_file_write);

#define PLATFORM_END_FILE(name) void name(game_file_handle Handle)
typedef PLATFORM_END_FILE(platform_end_file);

#define PLATFORM_READ_ENTIRE_FILE(name) file_read_result name(char *FileName)
typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);

#define PLATFORM_READ_FILE(name) file_read_result name(game_file_handle Handle, void *Memory, size_t Size, u32 Offset)
typedef PLATFORM_READ_FILE(platform_read_file);

#define PLATFORM_WRITE_FILE(name) void name(game_file_handle *Handle, void *Memory, size_t Size, u32 Offset)
typedef PLATFORM_WRITE_FILE(platform_write_file);

#define PLATFORM_FILE_SIZE(name) size_t name(char *FileName)
typedef PLATFORM_FILE_SIZE(platform_file_size);

inline void
ClearMemory(void *Memory, u32 Size, u8 ClearValue = 0)
{
    
    u8 *At = (u8 *)Memory;
    
    while(Size--)
    {
        *At++ = ClearValue;
    }
}

inline void
ClearMemoryU32(void *Memory, u32 Size, u32 ClearValue)
{
    
    u32 *At = (u32 *)Memory;
    
    while(Size--)
    {
        *At++ = ClearValue;
    }
}

#define CopyArray(Source, Dest, Type, Count) MemoryCopy(Source, Dest, sizeof(Type)*Count);

inline void
MemoryCopy(void *Source, void *Dest, u32 Size)
{
    u8 *Source_u8 = (u8 *)Source;
    u8 *Dest_u8 = (u8 *)Dest;
    
    while(Size--)
    {
        *Dest_u8++ = *Source_u8++;
        
    }
}

inline s32
MemoryCopy(void *NullTerminatedSource, void *Dest)
{
    u8 *NullTerminatedSource_u8 = (u8 *)NullTerminatedSource;
    u8 *Dest_u8 = (u8 *)Dest;
    
    s32 BytesWritten = 0;
    while(*NullTerminatedSource_u8)
    {
        *Dest_u8++ = *NullTerminatedSource_u8++;
        BytesWritten++;
    }
    
    return BytesWritten;
}


inline void
CopyStringToBuffer(char *Buffer, u32 BufferSize, char *StringToCopy)
{
    char *BufferAt = Buffer;
    u32 Count = 0;
    for(char *Letter = StringToCopy;
        *Letter;
        ++Letter)
    {
        *BufferAt++ = *Letter;
        Assert(Count < (BufferSize - 1));
        Count++;
    }
    
    *BufferAt = 0;
}

struct bitmap
{
    u32 Width;
    u32 Height;
    s32 Pitch;
    u32 Handle;
    v2 AlignPercent;
    
    void *Bits;
    
};

#define WasPressed(button_input) (button_input.IsDown && button_input.FrameCount > 0)
#define WasReleased(button_input) (!button_input.IsDown && button_input.FrameCount > 0)
#define IsDown(button_input) (button_input.IsDown)

#define BUTTON_INFO 1
enum game_button_type {
#include "meta_keys.h"
    
    //Make sure this is at the end
    Button_Count
    
};

struct game_button
{
    b32 IsDown;
    b32 FrameCount;
};

struct font;
struct game_memory
{
    void *GameStorage;
    u32 GameStorageSize;
    
    game_button *GameKeys;
    u32 SizeOfGameKeys;
    
    platform_read_entire_file *PlatformReadEntireFile;
    platform_read_file *PlatformReadFile;
    platform_write_file *PlatformWriteFile;
    platform_file_size *PlatformFileSize;
    platform_begin_file *PlatformBeginFile;
    platform_begin_file_write *PlatformBeginFileWrite;
    platform_end_file *PlatformEndFile;
    
    r32 MouseX;
    r32 MouseY;
    
    game_button GameButtons[Button_Count];
    
    b32 SoundOn;
    b32 *GameRunningPtr;
    
    // TODO(Oliver): Do we want the platform layer to load this? INstead of the game layer filling it in?
    font *DebugFont;
};

inline void EndProgram(game_memory *Memory) {
    *Memory->GameRunningPtr = false;
}

struct game_sound_buffer
{
    s16 *Samples;
    u32 ChannelCount;
    u32 SamplesToWrite;
    u32 SamplesPerSecond;
    
};

//MEMORY ARENAS


struct memory_arena
{
    void *Base;
    size_t CurrentSize;
    size_t TotalSize;
    
    u32 TempMemCount;
};

struct temp_memory
{
    memory_arena *Arena;
    size_t OriginalSize;
    
    u32 ID;
};

#define PushArray(Arena, Type, Size, ...) (Type *)PushSize(Arena, Size*sizeof(Type), __VA_ARGS__)
#define PushStruct(Arena, Type) (Type *)PushSize(Arena, sizeof(Type))


inline void *
PushSize(memory_arena *Arena, size_t Size, b32 Clear = true)
{
    Assert((Arena->CurrentSize + Size) <= Arena->TotalSize);
    void *Result = (void *)((u8 *)Arena->Base + Arena->CurrentSize);
    Arena->CurrentSize += Size;
    
    if(Clear)
    {
        size_t ClearSize = Size;
        u8 *Memory = (u8 *)Result; 
        while(ClearSize--)
        {
            *Memory++ = 0;
        }
    }    
    return Result;
    
}


inline void InitializeMemoryArena(memory_arena *Arena, void *Memory, size_t Size) {
    Arena->Base = (u8 *)Memory;
    Arena->CurrentSize = 0;
    Arena->TotalSize = Size;
} 

inline memory_arena SubMemoryArena(memory_arena *Arena, size_t Size) {
    memory_arena Result = {};
    
    Result.TotalSize = Size;
    Result.CurrentSize = 0;
    Result.Base = (u8 *)PushSize(Arena, Result.TotalSize);
    
    return Result;
}

internal u32 StringLength(char *A) {
    u32 Result = 0;
    
    while (*A++)
    {
        Result++;
    }
    
    return Result;
}

inline b32
DoStringsMatch(char *A, char *B)
{
    b32 Result = true;
    while(*A && *B)
    {
        Result &= (*A++ == *B++);
    }
    
    Result &= (!*A && !*B);
    
    return Result;
}
inline b32 DoStringsMatch(char *NullTerminatedA, char *B, u32 LengthOfB) {
    b32 Result = true;
    while(*NullTerminatedA && LengthOfB > 0) {
        Result &= (*NullTerminatedA++ == *B++);
        LengthOfB--;
    }
    Result &= (LengthOfB == 0) && (!*NullTerminatedA);
    
    return Result;
}

inline temp_memory
MarkMemory(memory_arena *Arena)
{
    temp_memory Result = {};
    Result.Arena = Arena;
    Result.OriginalSize = Arena->CurrentSize;
    Result.ID = Arena->TempMemCount++;
    
    return Result;
    
}

inline void
ReleaseMemory(temp_memory *TempMem)
{
    TempMem->Arena->CurrentSize = TempMem->OriginalSize;
    u32 ID = --TempMem->Arena->TempMemCount;
    Assert(TempMem->ID == ID);
    
}

inline void 
EmptyMemoryArena(memory_arena *Arena) {
    Assert(Arena->TempMemCount == 0);
    Arena->CurrentSize = 0;
}

inline char *Concat(memory_arena *Arena, char *A, char *B) {
    s32 Size = StringLength(A) + StringLength(B) + 1; //plus one for null terminated
    char *Data = (char *)PushSize(Arena, Size);
    char *DataAt = Data;
    while(*A) {
        *DataAt++ = *A++;
    }
    while(*B) {
        *DataAt++ = *B++;
    }
    
    *DataAt++ = '\0';
    
    Assert((DataAt - Data) == Size);
    
    return Data;
}

inline char *Concat(memory_arena *Arena, char *A, char *B, char *C) {
    char *AB = Concat(Arena, A, B);
    char *ABC = Concat(Arena, AB, C);
    return ABC;
    
    
}

#define CALM_PLATFORM_H
#endif

#if !defined(CALM_PLATFORM_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

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

typedef b32 bool32;

#define PI32 3.14159265359f
#define TAU32 6.28318530717958f

#include "calm_intrinsics.h"

#include "calm_math.h"

#include <float.h>

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
#define forN(Count) forN_(Count, ##Count##Index)


#if INTERNAL_BUILD
#define Assert(Condition) {if(!(Condition)){ u32 *A = (u32 *)0; *A = 0;}}
#else
#define Assert(Condition)
#endif

#define InvalidCodePath Assert(!"Invalid Code Path");


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

#define PLATFORM_END_FILE(name) void name(game_file_handle Handle)
typedef PLATFORM_END_FILE(platform_end_file);

#define PLATFORM_READ_ENTIRE_FILE(name) file_read_result name(char *FileName)
typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);

#define PLATFORM_READ_FILE(name) file_read_result name(game_file_handle Handle, void *Memory, size_t Size, u32 Offset)
typedef PLATFORM_READ_FILE(platform_read_file);

#define PLATFORM_FILE_SIZE(name) size_t name(char *FileName)
typedef PLATFORM_FILE_SIZE(platform_file_size);


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
    v2 AlignPercent;
    
    void *Bits;
    
};

#define WasPressed(button_input) (button_input.IsDown && button_input.FrameCount > 0)
#define WasReleased(button_input) (!button_input.IsDown && button_input.FrameCount > 0)
#define IsDown(button_input) (button_input.IsDown)

enum game_button_type
{
    Button_LeftMouse,
    Button_RightMouse,
    Button_Left,
    Button_Right,
    Button_Up,
    Button_Down,
    Button_Space,
    Button_Escape,
    Button_Enter,
    Button_Shift,
    
    //Make sure this is at the end
    Button_Count
    
};

struct game_button
{
    b32 IsDown;
    b32 FrameCount;
};

struct game_memory
{
    void *GameStorage;
    u32 GameStorageSize;
    
    platform_read_entire_file *PlatformReadEntireFile;
    platform_read_file *PlatformReadFile;
    platform_file_size *PlatformFileSize;
    platform_begin_file *PlatformBeginFile;
    platform_end_file *PlatformEndFile;
    
    r32 MouseX;
    r32 MouseY;
    
    game_button GameButtons[Button_Count];
    
    b32 SoundOn;
    b32 *GameRunningPtr;
};

struct game_sound_buffer
{
    s16 *Samples;
    u32 ChannelCount;
    u32 SamplesToWrite;
    u32 SamplesPerSecond;
    
};

#define CALM_PLATFORM_H
#endif

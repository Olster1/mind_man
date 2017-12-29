#if !defined(CALM_WIN32_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

struct offscreen_buffer
{
    BITMAPINFO BitmapInfo;
    u32 Width;
    u32 Height;
    void *Bits;
    
};

struct Win32SoundOutput{
    int32 sampleRate;
    int32 RunningSampleIndex;
    uint8 BytesPerSample;
    uint32 SizeOfBuffer;
    DWORD BytesPerFrame;
    uint32 SamplesPerFrame;
    uint32 SafetyBytes;
};

struct win32_dimension
{
    u32 X, Y;
};

#define CALM_WIN32_H
#endif

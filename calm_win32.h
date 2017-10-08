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

struct sound_info
{
    u32 NumberOfChannels;
    u32 BitsPerSample;
    u32 BytesPerSingleChannelSamples;
    u32 BytesPerSampleForBothChannels;
    u32 SamplesPerSec;
    u32 BytesPerSec;
    u32 BufferLengthInBytes;
    u32 BytesPerFrame;
    u32 BufferByteAt;
};

struct win32_dimension
{
    u32 X, Y;
};

#define CALM_WIN32_H
#endif

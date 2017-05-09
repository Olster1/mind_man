#if !defined(CALM_INTRINSICS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */
#include "math.h"

inline s32
CeilRealToInt32(r32 Value)
{
    s32 Result = (s32)ceil(Value);
    return Result;
}

inline s32
FloorRealToInt32(r32 Value)
{
    s32 Result = (s32)floor(Value);
    return Result;
}


inline r32
Sin(r32 Value)
{
    r32 Result = (r32)sin(Value);
    return Result;
}

inline r32
SqrRoot(r32 Value)
{
    r32 Result = (r32)sqrt(Value);
    return Result;
}

inline r32
Abs(r32 Value)
{
    r32 Result = (Value < 0.0f) ? Value*-1 : Value;
    return Result;
}

inline r32
Cos(r32 Value)
{
    r32 Result = (r32)cos(Value);
    return Result;
}

inline r32
InverseCos(r32 Value)
{
    r32 Result = (r32)acos(Value);
    return Result;
}

inline u32 TruncateReal32ToUInt32(r32 Real32)
{
    u32 Result = (u32)Real32;
    return Result;
}

inline r32 SignOf(r32 Value)
{
    r32 Result = (Value > 0.0f) ? 1.0f : -1.0f;
    return Result;
    
}

#define CALM_INTRINSICS_H
#endif

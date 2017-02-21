#if !defined(CALM_RENDER_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

enum render_element_type
{
    render_clear,
    render_bitmap,
    render_rect,
    render_rect_outline,
};

struct render_element_header
{
    render_element_type Type;
    
};

struct render_element_clear
{
    v4 Color;
};

struct render_element_bitmap
{
    v2 Pos;
    bitmap *Bitmap;
    rect2 ClipRect;
};

struct render_element_rect
{
    rect2 Dim;
    v4 Color;
};

struct render_element_rect_outline
{
    rect2 Dim;
    v4 Color;
};


struct render_group
{
    r32 MetersToPixels;
    v2 ScreenDim;
    
    bitmap *Buffer;
    
    temp_memory TempMem;
    memory_arena Arena;
    
};

#define M(Value, Index) (((real32 *)&Value)[Index])
#define Mi(Value, Index) (((int32 *)&Value)[Index])

#define CALM_RENDER_H
#endif

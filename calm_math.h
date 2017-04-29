#if !defined(CALM_MATH_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

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


struct v3
{
    union
    {
        struct
        {
            r32 X, Y, Z;
        };
        struct
        {
            r32 R, G, B;
        };
        struct
        {
            v2 XY;
            r32 Ignored0;
        };
        struct
        {
            r32 Ignored1;
            v2 YZ;
        };
    };
};

struct v4
{
    union
    {
        struct
        {
            r32 X, Y, Z, W;
        };
        struct
        {
            r32 R, G, B, A;
        };
    };
};

struct mat2
{
    union
    {
        r32 E[2][2];
        struct
        
        {
            r32 X1, Y1, X2, Y2;
        };
    };
};

#define Sqr(Value) ((r32)(Value)*(r32)(Value))

inline mat2
IdentityMatrix2()
{
    mat2 Result;
    
    Result.E[0][0] = 1; Result.E[0][1] = 0;
    Result.E[1][0] = 0; Result.E[1][1] = 1;
    
    return Result;
}

inline mat2
Mat2()
{
    mat2 Result = IdentityMatrix2();
    return Result;
}

inline v2
V2(r32 X, r32 Y)
{
    v2 Result;
    Result.X = X;
    Result.Y = Y;
    
    return Result;
}

inline v2i
V2int(s32 X, s32 Y)
{
    v2i Result;
    Result.X = X;
    Result.Y = Y;
    
    return Result;
}

inline v2
V2i(s32 X, s32 Y)
{
    v2 Result;
    Result.X = (r32)X;
    Result.Y = (r32)Y;
    
    return Result;
}

inline v3
V3(r32 X, r32 Y, r32 Z)
{
    v3 Result;
    Result.X = X;
    Result.Y = Y;
    Result.Z = Z;
    
    return Result;
}

inline v4
V4(r32 X, r32 Y, r32 Z, r32 W)
{
    v4 Result;
    Result.X = X;
    Result.Y = Y;
    Result.Z = Z;
    Result.W = W;
    
    return Result;
}

inline r32
SafeRatio0(r32 A, r32 B)
{
    r32 Result = 0;
    if(B != 0.0f)
    {
        Result = A / B;
    }
    
    return Result;
}

inline u32 
Power(u32 Value, u32 Exponent)
{
    u32 Result = 1;
    
    for(u32 LoopIndex = 0;
        LoopIndex < Exponent;
        ++LoopIndex)
    {
        Result *= Value;
    }
    
    return Result;
}

inline r32
TruncateFraction(r32 Value, u32 DecimalPlaces)
{
    u32 SafePortion = Power(10, DecimalPlaces);
    r32 Result = ((u32)(Value * SafePortion)) / (r32)SafePortion;
    return Result;
}

inline r32
Lerp(r32 A, r32 t, r32 B)
{
    r32 Result = A + t*(B - A);
    return Result;
}



inline r32 SineousLerp0To1(r32 A, r32 t, r32 B) {
    r32 TransformedT = sin(t*PI32/2);
    r32 Result = Lerp(A, TransformedT, B);
    return Result;
}

inline r32 SineousLerp0To0(r32 A, r32 t, r32 B) {
    r32 TransformedT = sin(t*PI32);
    r32 Result = Lerp(A, TransformedT, B);
    return Result;
}

inline r32 Clamp(r32 Min, r32 Value, r32 Max) {
    if(Value < Min) {
        Value = Min;
    }
    if(Value > Max) {
        Value = Max;
    }
    return Value;
}

inline s32
Abs(s32 Value)
{
    s32 Result = (Value < 0) ? Value*-1 : Value;
    return Result;
}

//NOTE(oliver): matrix2 operations
inline v2
Multiply(v2 A, mat2 B)
{
    v2 Result = {};
    
    Result.X = A.X * B.E[0][0] + A.Y * B.E[1][0];
    Result.Y = A.X * B.E[0][1] + A.Y * B.E[1][1];
    
    return Result;
}

inline mat2
Rotate(mat2 A, r32 Theta)
{
    mat2 Result = {};
    
    Result.E[0][0] = A.E[0][0] + A.E[0][1]*Theta;
    Result.E[0][1] = A.E[0][1] - A.E[0][0]*Theta;
    Result.E[1][0] = A.E[1][0] + A.E[1][1]*Theta;
    Result.E[1][1] = A.E[1][1] - A.E[1][0]*Theta;
    
    return Result;
}


//NOTE(oliver): V2 operations
inline r32
Inner(v2 A, v2 B)
{
    r32 Result = A.X*B.X + A.Y*B.Y;
    return Result;
}

inline v2
Perp(v2 A)
{
    v2 Result = V2(-A.Y, A.X);
    return Result;
}

inline v2i
Perp(v2i A)
{
    v2i Result = V2int(-A.Y, A.X);
    return Result;
}
inline v2
operator +(v2 A, v2 B)
{
    v2 Result;
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;
    return Result;
}


inline v2
operator -(v2 A, v2 B)
{
    v2 Result;
    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;
    return Result;
}

inline v2
operator -(v2 A)
{
    v2 Result;
    Result.X = -A.X;
    Result.Y = -A.Y;
    return Result;
}

inline v2 &
operator +=(v2 &A, v2 B)
{
    A = A + B;
    return A;
}

inline v2 &
operator -=(v2 &A, v2 B)
{
    A = A - B;
    return A;
}

inline v2
operator *(r32 Scalar, v2 A)
{
    v2 Result;
    Result.X = Scalar * A.X;
    Result.Y = Scalar * A.Y;
    
    return Result;
}

inline v2i
operator *(u32 Scalar, v2i A)
{
    v2i Result;
    Result.X = Scalar * A.X;
    Result.Y = Scalar * A.Y;
    
    return Result;
}

inline v2
operator /(v2 A, v2 B)
{
    v2 Result;
    Result.X = A.X / B.X;
    Result.Y = A.Y / B.Y;
    
    return Result;
}


inline v2 &
operator *=(v2 &A, r32 Scalar)
{
    A = Scalar*A;
    return A;
}

inline v2
Lerp(v2 A, r32 t, v2 B)
{
    v2 Result = A + t*(B - A);
    return Result;
}

inline v2 SineousLerp0To1(v2 A, r32 t, v2 B) {
    r32 TransformedT = sin(t*PI32/2);
    v2 Result = Lerp(A, TransformedT, B);
    return Result;
}

inline v2 SineousLerp0To0(v2 A, r32 t, v2 B) {
    r32 TransformedT = sin(t*PI32);
    v2 Result = Lerp(A, TransformedT, B);
    return Result;
}

inline b32
operator == (v2 A, v2 B)
{
    b32 Result = (A.X == B.X) && (A.Y == B.Y);
    return Result;
}

inline b32
operator == (v2i A, v2i B)
{
    b32 Result = (A.X == B.X) && (A.Y == B.Y);
    return Result;
}

inline b32
operator != (v2i A, v2i B)
{
    b32 Result = !((A.X == B.X) && (A.Y == B.Y));
    return Result;
}

inline b32
operator != (v2 A, v2 B)
{
    b32 Result = (A.X != B.X) || (A.Y != B.Y);
    return Result;
}

inline r32
Length(v2 A)
{
    r32 Result = SqrRoot(Sqr(A.X) + Sqr(A.Y));
    return Result;
}

inline r32
LengthSqr(v2 A)
{
    r32 Result = Inner(A, A);
    return Result;
}


inline v2
Normal(v2 A)
{
    v2 Result = (1.0f / Length(A))*A;
    return Result;
}
inline v2
Hadamard(v2 A, v2 B)
{
    v2 Result = V2(A.X*B.X, A.Y*B.Y);
    return Result;
}

inline v2
ToV2(v2i In)
{
    v2 Result = V2i(In.X, In.Y);
    return Result;
}

inline v2i
ToV2i(v2 In)
{
    v2i Result = V2int((s32)In.X, (s32)In.Y);
    return Result;
}

inline v2i
operator +(v2i A, v2i B)
{
    v2i Result = {};
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;
    return Result;
    
}

inline v2i
operator -(v2i A, v2i B)
{
    v2i Result = {};
    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;
    return Result;
    
}

inline v2
Abs(v2 Value)
{
    v2 Result = {};
    Result.X = (Value.X < 0.0f) ? Value.X*-1 : Value.X;
    Result.Y = (Value.Y < 0.0f) ? Value.Y*-1 : Value.Y;
    return Result;
}

//NOTE(oliver): V3 operations

inline v3
operator *(r32 Scalar, v3 A)
{
    v3 Result;
    Result.X = Scalar * A.X;
    Result.Y = Scalar * A.Y;
    Result.Z = Scalar * A.Z;
    
    return Result;
}

inline v3 &
operator *=(v3 &A, r32 Scalar)
{
    A = Scalar*A;
    return A;
}

//NOTE(oliver): V4 operations

inline v4
operator +(v4 A, v4 B)
{
    v4 Result;
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;
    Result.Z = A.Z + B.Z;
    Result.W = A.W + B.W;
    
    return Result;
}

inline v4
operator -(v4 A, v4 B)
{
    v4 Result;
    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;
    Result.Z = A.Z - B.Z;
    Result.W = A.W - B.W;
    
    return Result;
}

inline v4
operator *(r32 Scalar, v4 A)
{
    v4 Result;
    Result.X = Scalar * A.X;
    Result.Y = Scalar * A.Y;
    Result.Z = Scalar * A.Z;
    Result.W = Scalar * A.W;
    
    return Result;
}
inline v4 &
operator +=(v4 &A, v4 B)
{
    A = A + B;
    return A;
}


inline v4
Lerp(v4 Start, r32 t, v4 End)
{
    v4 Result = Start + t*(End - Start);
    return Result;
}

inline v4
Hadamard(v4 A, v4 B)
{
    v4 Result = V4(A.X*B.X, A.Y*B.Y, A.Z*B.Z, A.W*B.W);
    return Result;
}

inline v4 
SineousLerp0To1(v4 A, r32 tValue, v4 B) {
    
    r32 X = SineousLerp0To1(A.R, tValue, B.R);
    r32 Y = SineousLerp0To1(A.G, tValue, B.G);
    r32 Z = SineousLerp0To1(A.B, tValue, B.B);
    r32 W = SineousLerp0To1(A.A, tValue, B.A);
    v4 Result = V4(X, Y, Z, W); 
    return Result;
    
}

inline v4 
SineousLerp0To0(v4 A, r32 tValue, v4 B) {
    
    r32 X = SineousLerp0To0(A.R, tValue, B.R);
    r32 Y = SineousLerp0To0(A.G, tValue, B.G);
    r32 Z = SineousLerp0To0(A.B, tValue, B.B);
    r32 W = SineousLerp0To0(A.A, tValue, B.A);
    v4 Result = V4(X, Y, Z, W); 
    return Result;
    
}

//NOTE(Oliver): rect2 operations

struct rect2
{
    union
    {
        struct
        {
            r32 MinX;
            r32 MinY;
            r32 MaxX;
            r32 MaxY;
        };
        struct
        {
            v2 Min;
            v2 Max;
        };
    };
};

inline rect2
Rect2(r32 MinX, r32 MinY, r32 MaxX, r32 MaxY)
{
    rect2 Result = {};
    Result.MinX = MinX;
    Result.MinY = MinY;
    Result.MaxX = MaxX;
    Result.MaxY = MaxY;
    
    return Result;    
}

inline rect2
Rect2MinMax(v2 Min, v2 Max)
{
    rect2 Result = {};
    Result.Min = Min;
    Result.Max = Max;
    
    return Result;    
}

inline rect2
Rect2CenterDim(v2 Center, v2 Dim)
{
    rect2 Result = {};
    
    v2 HalfDim = 0.5f*Dim;
    
    Result.Min.X = Center.X - HalfDim.X;
    Result.Min.Y = Center.Y - HalfDim.Y,
    Result.Max.X = Center.X + HalfDim.X,
    Result.Max.Y = Center.Y + HalfDim.Y;
    
    return Result;
}

inline rect2
Rect2OriginCenterDim(v2 Dim)
{
    rect2 Result = Rect2CenterDim(V2(0, 0), Dim);
    return Result;
}

inline v2
GetMinCorner(rect2 Rect)
{
    v2 Result = Rect.Min;
    return Result;
}

inline v2
GetMaxCorner(rect2 Rect)
{
    v2 Result = Rect.Max;
    return Result;
}

inline b32
InBounds(rect2 Rect, v2 Point)
{
    b32 Result = (Point.X >= Rect.Min.X &&
                  Point.Y >= Rect.Min.Y &&
                  Point.X < Rect.Max.X &&
                  Point.Y < Rect.Max.Y);
    return Result;
}

inline r32 GetWidth(rect2 Rect) 
{
    r32 Result = Rect.MaxX - Rect.MinX;
    return Result;
}

inline r32 GetHeight(rect2 Rect) 
{
    r32 Result = Rect.MaxY - Rect.MinY;
    return Result;
}

#define LARGE_NUMBER 100000.0f


inline rect2
InvertedInfinityRectangle()
{
    rect2 Result = {};
    Result.Min = V2(LARGE_NUMBER, LARGE_NUMBER);
    Result.Max = V2(-LARGE_NUMBER, -LARGE_NUMBER);
    
    return Result;
}

inline rect2
Union(rect2 A, rect2 B)
{
    rect2 Result = {};
    
    Result.Min.X = (A.Min.X < B.Min.X) ? A.Min.X : B.Min.X;
    Result.Min.Y = (A.Min.Y < B.Min.Y) ? A.Min.Y : B.Min.Y;
    Result.Max.X = (A.Max.X > B.Max.X) ? A.Max.X : B.Max.X;
    Result.Max.Y = (A.Max.Y > B.Max.Y) ? A.Max.Y : B.Max.Y;
    
    return Result;
    
}

b32
operator ==(rect2 A, rect2 B)
{
    b32 Result = (A.Min.X == B.Min.X &&
                  A.Max.Y == B.Max.Y &&
                  A.Min.X == B.Min.X &&
                  A.Max.Y == B.Max.Y);
    
    return Result;
}

#define CALM_MATH_H
#endif

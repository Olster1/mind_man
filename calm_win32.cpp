/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#define WRITE_FONT_FILE 0
#define SWAP_BUFFER_INTERVAL 1

#include <windows.h>
#include <stdio.h>

#include "calm_platform.h"
#include "calm_win32.h"
#include "calm_render.h"
#include "calm_font.h"
#if WRITE_FONT_FILE
#include "write_font_file_win32.h"
#endif
#include "dsound.h"
#include <gl/gl.h>

global_variable b32 GlobalRunning;

global_variable offscreen_buffer GlobalOffscreenBuffer;
global_variable LARGE_INTEGER GlobalTimeFrequencyDatum;
global_variable LPDIRECTSOUNDBUFFER Global_SecondaryBuffer;

global_variable GLuint GlobalOpenGlDefaultInternalTextureFormat;

#include "calm_opengl.cpp"

global_variable int OpenGLAttribs[] = {
    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    WGL_CONTEXT_MINOR_VERSION_ARB, 0,
    WGL_CONTEXT_FLAGS_ARB, 0 // NOTE(OLIVER): Enable For testing WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#if _INTERNAL
    |WGL_CONTEXT_DEBUG_BIT_ARB
#endif
    ,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
    0
};

#include "calm_game.cpp"

typedef BOOL WINAPI wgl_swap_interval_ext(int interval);
global_variable wgl_swap_interval_ext *wglSwapInterval;

typedef  HGLRC wgl_Create_Context_Attribs_ARB(HDC hDC, HGLRC hShareContext, const int *attribList);

wgl_Create_Context_Attribs_ARB *Global_wglCreateContextAttribsARB;

#define DIRECT_SOUND_CREATE(name) HRESULT name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void 
Win32BltBitmapToScreen(HDC DestDC, u32 WindowWidth, u32 WindowHeight)
{
    u32 BufferWidth = GlobalOffscreenBuffer.Width;
    u32 BufferHeight = GlobalOffscreenBuffer.Height;
    
    u32 XOffset = 0;
    u32 YOffset = 0;
    
    PatBlt(DestDC, 0, 0, WindowWidth, YOffset, BLACKNESS);
    PatBlt(DestDC, 0, 0, XOffset, WindowHeight, BLACKNESS);
    
    PatBlt(DestDC, BufferWidth + XOffset, 0, WindowWidth, WindowHeight, BLACKNESS);
    PatBlt(DestDC, 0, BufferHeight + YOffset, WindowWidth, WindowHeight, BLACKNESS);
    
    StretchDIBits(DestDC,
                  XOffset,
                  YOffset,
                  BufferWidth,
                  BufferHeight,
                  0,
                  0,
                  BufferWidth,
                  BufferHeight,
                  GlobalOffscreenBuffer.Bits,
                  &GlobalOffscreenBuffer.BitmapInfo,
                  DIB_RGB_COLORS,
                  SRCCOPY);
    
}

inline win32_dimension
Win32GetClientDimension(HWND WindowHandle)
{
    win32_dimension Result = {};
    
    RECT WindowRect;
    GetClientRect(WindowHandle, &WindowRect);
    Result.X = WindowRect.right - WindowRect.left;
    Result.Y = WindowRect.bottom - WindowRect.top;
    
    return Result;
}

inline void UpdateButton(game_button *Button, b32 IsUp, b32 WasDown) {
    Button->IsDown = !IsUp;
    if(WasDown == IsUp) {
        Button->FrameCount++;
    }
} 

global_variable game_button GameKeys[256 + 4]; // + four extra places for keyup, down, left, right etc.

LRESULT CALLBACK
Win32MainWindowCallBack(
HWND Handle,
UINT Msg,
WPARAM wParam,
LPARAM lParam)

{
    LRESULT Result = 0;
    
    switch(Msg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT PaintInfo;
            HDC WindowDC = BeginPaint(Handle, &PaintInfo);
            
            win32_dimension WindowDim = Win32GetClientDimension(Handle);
            
            Win32BltBitmapToScreen(WindowDC, WindowDim.X, WindowDim.Y);
            
            EndPaint(Handle, &PaintInfo);
            
            
        } break;
        case WM_CHAR: {
            b32 IsUp = lParam & (1 << 31);
            b32 WasDown = lParam & (1 << 30);
            
            game_button *Button = GameKeys + wParam;
            UpdateButton(Button, IsUp, WasDown);
            
        } break;
        case WM_SIZE:
        {
            
            
        } break;
        
        
        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;
        case WM_ACTIVATEAPP:
        {
            
        }
        
        
        
        default:
        {
            Result = DefWindowProc(Handle, Msg, wParam, lParam);
        }
        
    }
    
    return Result;
}


inline s64
Win32GetTimeCount()
{
    LARGE_INTEGER Time;
    QueryPerformanceCounter(&Time);
    
    return Time.QuadPart;
}

inline r32
Win32GetSecondsElapsed(s64 CurrentCount, s64 LastCount)
{
    
    s64 Difference = CurrentCount - LastCount;
    r32 Seconds = (r32)Difference / (r32)GlobalTimeFrequencyDatum.QuadPart;
    
    return Seconds;
    
}

PLATFORM_BEGIN_FILE(Win32BeginFile)
{
    game_file_handle Result = {};
    
    HANDLE FileHandle = CreateFile(FileName,
                                   GENERIC_READ,
                                   FILE_SHARE_READ,
                                   0,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   0);
    
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        Result.Data = FileHandle;
    }
    else
    {
        Result.HasErrors = true;
    }
    
    return Result;
}

PLATFORM_BEGIN_FILE_WRITE(Win32BeginFileWrite)
{
    game_file_handle Result = {};
    
    HANDLE FileHandle = CreateFile(FileName,
                                   GENERIC_WRITE,
                                   FILE_SHARE_READ,
                                   0,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   0);
    
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        Result.Data = FileHandle;
    }
    else
    {
        DWORD Error = GetLastError();
        Result.HasErrors = true;
    }
    
    return Result;
}

PLATFORM_END_FILE(Win32EndFile)
{
    HANDLE FileHandle = (HANDLE)Handle.Data;
    if(FileHandle)
    {
        CloseHandle(FileHandle);
    }
}

PLATFORM_WRITE_FILE(Win32WriteFile)
{
    Handle->HasErrors = false;
    HANDLE FileHandle = (HANDLE)Handle->Data;
    if(!Handle->HasErrors)
    {
        Handle->HasErrors = true; // we set this here to avoid having to do elses and set 'HasErrors' to true
        
        if(FileHandle)
        {
            if(SetFilePointer(FileHandle, Offset, 0, FILE_BEGIN) !=  INVALID_SET_FILE_POINTER)
            {
                DWORD BytesWritten;
                if(WriteFile(FileHandle, Memory, (DWORD)Size, &BytesWritten, 0))
                {
                    if(BytesWritten == Size) {
                        Handle->HasErrors = false;
                    } 
                }
                else
                {
                    Assert(!"Read file did not succeed");
                }
            }
        }
    }    
}

PLATFORM_READ_FILE(Win32ReadFile)
{
    file_read_result Result = {};
    
    HANDLE FileHandle = (HANDLE)Handle.Data;
    if(!Handle.HasErrors)
    {
        if(FileHandle)
        {
            if(SetFilePointer(FileHandle, Offset, 0, FILE_BEGIN) !=  INVALID_SET_FILE_POINTER)
            {
                DWORD BytesRead;
                if(ReadFile(FileHandle, Memory, (DWORD)Size, &BytesRead, 0))
                {
                    Result.Data = Memory;
                    Result.Size = (u32)Size;
                }
                else
                {
                    Assert(!"Read file did not succeed");
                }
            }
        }
    }    
    return Result;
    
}

PLATFORM_FILE_SIZE(Win32FileSize)
{
    size_t Result = 0;
    
    HANDLE FileHandle = CreateFile(FileName,
                                   GENERIC_READ,
                                   FILE_SHARE_READ,
                                   0,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   0);
    
    if(FileHandle)
    {
        Result = GetFileSize(FileHandle, 0);
        CloseHandle(FileHandle);
    }
    
    return Result;
}

PLATFORM_READ_ENTIRE_FILE(Win32ReadEntireFile)
{
    file_read_result Result = {};
    
    HANDLE FileHandle = CreateFile(FileName,
                                   GENERIC_READ,
                                   FILE_SHARE_READ,
                                   0,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   0);
    
    if(FileHandle)
    {
        Result.Size = GetFileSize(FileHandle, 0);
        Result.Data = VirtualAlloc(0, Result.Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        
        DWORD BytesRead = {};
        
        if(ReadFile(FileHandle, Result.Data, Result.Size, &BytesRead, 0))
        {
            //NOTE(Oliver): Successfully read
        }
        else
        {
            Assert(!"Couldn't read file");
            
            VirtualFree(Result.Data, 0, MEM_RELEASE);
        }
    }
    
    CloseHandle(FileHandle);
    
    return Result;
}

internal HGLRC
Win32InitOpenGL(HDC WindowDC) {
    HGLRC Result = {};
    
    //NOTE: Pixel format we would like.
    PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
    DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
    DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
    DesiredPixelFormat.nVersion = 1;
    DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
    DesiredPixelFormat.cColorBits = 24;
    DesiredPixelFormat.cAlphaBits = 8;
    DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;
    
    //
    //NOTE(Ollie): We have to do this because the graphics card only has certain 
    //operations it can perfrom, so we have to negoitate with it and find a 
    //format that it supports
    //
    int SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex, sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
    
    HGLRC RenderContext = wglCreateContext(WindowDC); 
    
    if(wglMakeCurrent(WindowDC, RenderContext)) {
        
        b32 IsModernContext = false;
        Global_wglCreateContextAttribsARB =(wgl_Create_Context_Attribs_ARB *)wglGetProcAddress("wglCreateContextAttribsARB");
        
        if(Global_wglCreateContextAttribsARB) {
            // NOTE(OLIVER): Modern version of OpenlGl supported
            
            HGLRC ShareContext = 0;
            HGLRC ModernContext = Global_wglCreateContextAttribsARB(WindowDC, ShareContext, OpenGLAttribs);
            if(ModernContext) {
                if(wglMakeCurrent(WindowDC, ModernContext)) {
                    wglDeleteContext(RenderContext);
                    RenderContext = ModernContext;
                    IsModernContext = true;
                    Result = ModernContext;
                }
                
            } else {
                InvalidCodePath;
            }
        } else{
            // NOTE(OLIVER): This is an antiquated version of OpenGl i.e. below version 3?
            //Don't have to do anything. 
        }
        
        OpenGlInit(IsModernContext);
        
        wglSwapInterval =(wgl_swap_interval_ext *)wglGetProcAddress("wglSwapIntervalEXT");
        if(wglSwapInterval) {
            wglSwapInterval(SWAP_BUFFER_INTERVAL); //1 frame we hold before overriding -> I think this is the backup scheme. When the CPU gets back to this position it will have to wait till it is finished rendering -> I think this is V-sync
        }
    } else {
        InvalidCodePath
    }
    return Result;
}
internal void
Win32ClearSoundBuffer(Win32SoundOutput *SoundOutput){
    
    VOID *Address1;
    DWORD RegionSize1;
    VOID *Address2;
    DWORD RegionSize2;
    
    if(SUCCEEDED(Global_SecondaryBuffer->Lock(
        0, SoundOutput->SizeOfBuffer,
        &Address1, &RegionSize1,
        &Address2, &RegionSize2, 0))){
        
        
        uint8 *startAddress = (uint8 *)Address1;
        for(uint32 sampleIndex = 0;
            sampleIndex < RegionSize1;
            sampleIndex++){
            
            *startAddress++ = 0;
        }
        
        startAddress = (uint8 *)Address2;
        for(uint32 sampleIndex = 0;
            sampleIndex < RegionSize2;
            sampleIndex++){
            
            *startAddress++ = 0;
        }
        Global_SecondaryBuffer->Unlock(Address1, RegionSize1, Address2, RegionSize2);
    }           
}
internal void
Win32FillSoundBuffer(Win32SoundOutput *SoundOutput, game_output_sound_buffer *SourceBuffer, DWORD BytesToWrite, DWORD ByteToLock){
    
    VOID *Address1;
    DWORD RegionSize1;
    VOID *Address2;
    DWORD RegionSize2;
    
    if(SUCCEEDED(Global_SecondaryBuffer->Lock(
        ByteToLock, BytesToWrite,
        &Address1, &RegionSize1,
        &Address2, &RegionSize2, 0))){
        
        
        int16 *SourceSample = SourceBuffer->Samples;
        uint16 *StartAddress = (uint16 *)Address1;
        int32 RegionSampleSize = (int32)RegionSize1 / SoundOutput->BytesPerSample; 
        for(int32 sampleIndex = 0;
            sampleIndex < RegionSampleSize;
            sampleIndex++){
            
            *StartAddress++ = *SourceSample++;
            *StartAddress++ = *SourceSample++;
            SoundOutput->RunningSampleIndex++;
            
        }
        
        StartAddress = (uint16 *)Address2;
        RegionSampleSize = (int32)RegionSize2 / SoundOutput->BytesPerSample; 
        for(int32 sampleIndex = 0;
            sampleIndex < RegionSampleSize;
            sampleIndex++){
            
            *StartAddress++ = *SourceSample++;
            *StartAddress++ = *SourceSample++;
            SoundOutput->RunningSampleIndex++;
            
        }
        Global_SecondaryBuffer->Unlock(Address1, RegionSize1, Address2, RegionSize2);
        
    }           
    
}
internal void
initDSound(HWND Window, uint32 SamplesPerSecond, uint32 sizeOfBuffer){
    
    HMODULE dSoundLibrary = LoadLibraryA("dsound.dll"); //load library
    
    if(dSoundLibrary){ //if found library
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(dSoundLibrary, "DirectSoundCreate"); //get function & cast it
        
        LPDIRECTSOUND DirectSound;
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))){//if we found the method, we want to create a directSoundObject
            if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_EXCLUSIVE))){ //we set it eith the priority flag, meaning we can change the output format of the soundcard
                
                WAVEFORMATEX waveFormat = {};//set format of sound
                waveFormat.wFormatTag = WAVE_FORMAT_PCM;
                waveFormat.nChannels = 2;
                waveFormat.wBitsPerSample = 16;
                waveFormat.nSamplesPerSec = SamplesPerSecond;
                waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
                waveFormat.nAvgBytesPerSec = SamplesPerSecond * waveFormat.nBlockAlign;
                waveFormat.cbSize = 0;
                
                
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER; //type of buffer we want; we want the sound card handle first
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if(SUCCEEDED((DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))){ //grab the sound card and put it in our pointer
                    
                    if(SUCCEEDED(PrimaryBuffer->SetFormat(&waveFormat))){//set the format of the sound card
                        OutputDebugStringA("primary format set\n");
                    }
                    else{
                        
                    }
                }
                else{
                }
                
                BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_GLOBALFOCUS;
                BufferDescription.lpwfxFormat = &waveFormat;
                BufferDescription.dwBufferBytes = sizeOfBuffer; 
                if(SUCCEEDED((DirectSound->CreateSoundBuffer(&BufferDescription, &Global_SecondaryBuffer, 0)))){ //create our second buffer; the one we are going to write to
                    OutputDebugStringA("Secondary format set \n");
                }
                else{
                    
                }
            }
        }
        else{
        }
    }
    else{
        
    }
}

#define GetButtonState(...)

#include <stdarg.h>
inline void
GetButtonState_(game_button *Button, u32 NumberOfKeys, ...)
{
    b32 IsDown = false;
    
    va_list KeyFlags;
    va_start(KeyFlags, NumberOfKeys);
    fori_count(NumberOfKeys)
    {
        IsDown |= GetAsyncKeyState(va_arg(KeyFlags, u32)) >> 16;
        
    }
    va_end(KeyFlags);
    
    if(Button->IsDown != IsDown)
    {
        Button->FrameCount++;
    }
    
    Button->IsDown = IsDown;
}

//TODO(ollie): Make safe for threads other than the main thread to add stuff
PLATFORM_PUSH_WORK_ONTO_QUEUE(Win32PushWorkOntoQueue) {
    for(;;)
    {
        u32 OnePastTheHead = (Info->IndexToAddTo + 1) % ArrayCount(Info->WorkQueue);
        if(Info->WorkQueue[Info->IndexToAddTo].Finished && OnePastTheHead != Info->IndexToTakeFrom)
        {
            thread_work *Work = Info->WorkQueue + Info->IndexToAddTo; 
            Work->FunctionPtr = WorkFunction;
            Work->Data = Data;
            Work->Finished = false;
            
            MemoryBarrier();
            _ReadWriteBarrier();
            
            ++Info->IndexToAddTo %= ArrayCount(Info->WorkQueue);
            
            MemoryBarrier();
            _ReadWriteBarrier();
            
            ReleaseSemaphore(Info->Semaphore, 1, 0);
            
            break;
        }
        else
        {
            //NOTE(ollie): Queue is full
        }
    }
    
}

internal thread_work *
GetWorkOffQueue(thread_info *Info, thread_work **WorkRetrieved)
{
    *WorkRetrieved = 0;
    
    u32 OldValue = Info->IndexToTakeFrom;
    if(OldValue != Info->IndexToAddTo)
    {
        u32 NewValue = (OldValue + 1) % ArrayCount(Info->WorkQueue);
        if(InterlockedCompareExchange(&Info->IndexToTakeFrom, NewValue, OldValue) == OldValue)
        {
            *WorkRetrieved = Info->WorkQueue + OldValue;
        }
    }    
    return *WorkRetrieved;
}

internal void
Win32DoThreadWork(thread_info *Info)
{
    thread_work *Work;
    while(GetWorkOffQueue(Info, &Work))
    {
        Work->FunctionPtr(Work->Data);
        Assert(!Work->Finished);
        
        MemoryBarrier();
        _ReadWriteBarrier();
        
        Work->Finished = true;
        
    }
}

internal DWORD
Win32ThreadEntryPoint(LPVOID Info_)
{
    thread_info *Info = (thread_info *)Info_;
    
    if(Global_wglCreateContextAttribsARB) {
        Assert(Info->WindowDC);
        Assert(Info->ContextForThread);
        if(wglMakeCurrent(Info->WindowDC, Info->ContextForThread)) {
            
        } else {
            InvalidCodePath;
        }
    } 
    
    for(;;)
    {
        Win32DoThreadWork(Info);
        
        WaitForSingleObject(Info->Semaphore, INFINITE);
    }
    
}

inline b32
IsWorkFinished(thread_info *Info)
{
    b32 Result = true;
    for(u32 WorkIndex = 0;
        WorkIndex < ArrayCount(Info->WorkQueue);
        ++WorkIndex)
    {
        Result &= Info->WorkQueue[WorkIndex].Finished;
        if(!Result) { break; }
    }
    
    return Result;
}

internal void
WaitForWorkToFinish(thread_info *Info)
{
    while(!IsWorkFinished(Info))
    {
        Win32DoThreadWork(Info);        
    }
}

THREAD_WORK_FUNCTION(PrintTest)
{
    char *String = (char *)Data;
    OutputDebugString(String);
}

THREAD_WORK_FUNCTION(MessageLoop)
{
    HWND WindowHandle = (HWND)(Data);
    MSG Message;
    for(;;)
    {
        while(PeekMessage(&Message, WindowHandle, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
    }
}

int WinMain(HINSTANCE Instance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    
    WNDCLASSA WndClassInfo = {};
    WndClassInfo.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WndClassInfo.lpfnWndProc = Win32MainWindowCallBack;
    WndClassInfo.hInstance = Instance;
    WndClassInfo.lpszClassName = "Calm_window_class";
    
    if(RegisterClass(&WndClassInfo))
    {
#define FULL_SCREEN 0
#if FULL_SCREEN
        u32 Width = 1920;
        u32 Height = 1080;
#else 
        u32 Width = 960;
        u32 Height = 540;
#endif
        
        
        HWND WindowHandle = CreateWindowExA(0,
                                            WndClassInfo.lpszClassName,
                                            "Calm_app_window",
                                            WS_VISIBLE|WS_OVERLAPPEDWINDOW,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            0,
                                            0,
                                            Instance, 
                                            0);
        
        if(WindowHandle)
        {
#if FULL_SCREEN
            
            SetWindowPos(WindowHandle, HWND_TOPMOST, 0,  0, GetSystemMetrics(SM_CXSCREEN ), GetSystemMetrics( SM_CYSCREEN ), 0L );
#endif
            GlobalRunning = true;
            
            ShowCursor(FALSE);
            
            HDC WindowDC = GetDC(WindowHandle);
            HGLRC MainRenderContext = Win32InitOpenGL(WindowDC);
            
            SYSTEM_INFO SystemInfo;
            GetSystemInfo(&SystemInfo);
            
            u32 NumberOfProcessors = SystemInfo.dwNumberOfProcessors;
            
            u32 NumberOfUnusedProcessors = (NumberOfProcessors - 1); //NOTE(oliver): minus one to account for the one we are on
            
            thread_info ThreadInfo = {};
            ThreadInfo.Semaphore = CreateSemaphore(0, 0, NumberOfUnusedProcessors, 0);
            ThreadInfo.IndexToTakeFrom = ThreadInfo.IndexToAddTo = 0;
            ThreadInfo.WindowHandle = WindowHandle;
            ThreadInfo.WindowDC = WindowDC;
            
            for(u32 WorkIndex = 0;
                WorkIndex < ArrayCount(ThreadInfo.WorkQueue);
                ++WorkIndex)
            {
                ThreadInfo.WorkQueue[WorkIndex].Finished = true;
            }
            
            HANDLE Threads[12];
            u32 ThreadCount = 0;
            
            s32 CoreCount = Min(NumberOfUnusedProcessors, ArrayCount(Threads));
            
            for(u32 CoreIndex = 0;
                CoreIndex < (u32)CoreCount;
                ++CoreIndex)
            {
                HGLRC ContextForThisThread =  Global_wglCreateContextAttribsARB(WindowDC, MainRenderContext, OpenGLAttribs);
                
                ThreadInfo.ContextForThread = ContextForThisThread;
                Assert(ThreadCount < ArrayCount(Threads));
                Threads[ThreadCount++] = CreateThread(0, 0, Win32ThreadEntryPoint, &ThreadInfo, 0, 0);
            }
            
            QueryPerformanceFrequency(&GlobalTimeFrequencyDatum);
            
            timeBeginPeriod(1);
            
            ///////Render Groups we pass to the game layer/////
            render_group RenderGroup;
            render_group OrthoRenderGroup;
            
            u32 TargetFramesPerSecond = 60 / SWAP_BUFFER_INTERVAL;
            r32 TargetTimePerFrameInSeconds = 1.0f / (r32)TargetFramesPerSecond;
            
            if(GlobalOffscreenBuffer.Bits)
            {
                VirtualFree(GlobalOffscreenBuffer.Bits, 0, MEM_RELEASE);
            }
            
            GlobalOffscreenBuffer.Width = Width;
            GlobalOffscreenBuffer.Height = Height;
            u32 BufferMemorySize = Width * Height * sizeof(u32);
            
            GlobalOffscreenBuffer.Bits = VirtualAlloc(0,
                                                      BufferMemorySize,
                                                      MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            
            
            BITMAPINFOHEADER BitmapHeader = {};
            BitmapHeader.biSize = sizeof(BITMAPINFOHEADER);
            BitmapHeader.biWidth = GlobalOffscreenBuffer.Width;
            BitmapHeader.biHeight = GlobalOffscreenBuffer.Height; //NOTE(oliver): Bottom up bitmap
            BitmapHeader.biPlanes = 1;
            BitmapHeader.biBitCount = 32;
            BitmapHeader.biCompression = BI_RGB;
            BitmapHeader.biSizeImage = 0;
            
            GlobalOffscreenBuffer.BitmapInfo = {};
            GlobalOffscreenBuffer.BitmapInfo.bmiHeader = BitmapHeader;
            
            bitmap FrameBufferBitmap;
            FrameBufferBitmap.Width = GlobalOffscreenBuffer.Width;
            FrameBufferBitmap.Height = GlobalOffscreenBuffer.Height;
            FrameBufferBitmap.Pitch = GlobalOffscreenBuffer.Width*sizeof(u32);
            FrameBufferBitmap.Bits = GlobalOffscreenBuffer.Bits;
            
            //NOTE(ollie) settings for sound output
            Win32SoundOutput soundOutput = {};
            soundOutput.sampleRate = 48000;
            soundOutput.BytesPerSample = 4;
            soundOutput.SizeOfBuffer = Align8(soundOutput.sampleRate) * soundOutput.BytesPerSample*2;
            soundOutput.BytesPerFrame = (uint32)(soundOutput.sampleRate * TargetTimePerFrameInSeconds) * soundOutput.BytesPerSample;
            soundOutput.SafetyBytes = 2*(soundOutput.BytesPerFrame);
            
            int16 *soundSamples = (int16 *)VirtualAlloc(0, soundOutput.SizeOfBuffer, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            
            initDSound(WindowHandle, soundOutput.sampleRate, soundOutput.SizeOfBuffer);
            Win32ClearSoundBuffer(&soundOutput);    
            Global_SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            
            game_memory GameMemory = {};
            GameMemory.GameStorageSize = GigaBytes(1);
            GameMemory.GameKeys = GameKeys;
            GameMemory.SizeOfGameKeys = ArrayCount(GameKeys);
            
            
            u32 TotalStorageSize = GameMemory.GameStorageSize;
            
            GameMemory.GameStorage = VirtualAlloc(0,
                                                  TotalStorageSize,
                                                  MEM_COMMIT | MEM_RESERVE,
                                                  PAGE_READWRITE);
            
            GameMemory.PlatformReadEntireFile = Win32ReadEntireFile;
            GameMemory.PlatformReadFile = Win32ReadFile;
            GameMemory.PlatformWriteFile = Win32WriteFile;
            GameMemory.PlatformFileSize = Win32FileSize;
            GameMemory.PlatformBeginFile = Win32BeginFile;
            GameMemory.PlatformBeginFileWrite = Win32BeginFileWrite;
            GameMemory.PlatformPushWorkOntoQueue = Win32PushWorkOntoQueue;
            GameMemory.PlatformEndFile = Win32EndFile;
            GameMemory.GameRunningPtr = &GlobalRunning;
            GameMemory.SoundOn = false;
            GameMemory.ThreadInfo = &ThreadInfo;
            
#if WRITE_FONT_FILE
            Win32WriteFontFile();
#endif
            s64 LastCounter = Win32GetTimeCount();
            
            s32 PlayCursorIndex = 0;
            s32 WriteCursorIndex = 0;
            s32 CursorAtIndex = 0;
            s32 FrameFlipAtIndex = 0;
            
            s32 WriteCursors[512] = {};
            s32 PlayCursors[512] = {};
            s32 CursorsAt[512] = {};
            s32 FrameFlipAt[512] = {};
            s32 ViewIndex = 0;
            
            b32 SoundIsValid = false;
            b32 FirstTimeInGameLoop = true;
            s64 AudioFlipWallClock = Win32GetTimeCount();
            while(GlobalRunning)
            {
                ClearMemory(GameKeys, sizeof(GameKeys));
                
                MSG Message;
                while(PeekMessage(&Message, WindowHandle, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                
                
                for(u32 Index = 0;
                    Index < ArrayCount(GameMemory.GameButtons);
                    Index++)
                {
                    GameMemory.GameButtons[Index].FrameCount = 0;                   
                }
                
                GetButtonState(GameMemory.GameButtons + Button_Up, VK_UP, 0x57);
                GetButtonState(GameMemory.GameButtons + Button_Down, VK_DOWN, 0x53);
                GetButtonState(GameMemory.GameButtons + Button_Left, VK_LEFT, 0x41);
                GetButtonState(GameMemory.GameButtons + Button_Right, VK_RIGHT, 0x44);
                //////////////////
                // TODO(OLIVER): Replace this using the asci array in order to get the nice lag time on key press
                GetButtonState(GameMemory.GameButtons + Button_Arrow_Left, VK_LEFT);
                GetButtonState(GameMemory.GameButtons + Button_Arrow_Right, VK_RIGHT);
                ////////////////
                GetButtonState(GameMemory.GameButtons + Button_Space, VK_SPACE);
                GetButtonState(GameMemory.GameButtons + Button_Escape, VK_ESCAPE);
                GetButtonState(GameMemory.GameButtons + Button_Enter, VK_RETURN);
                GetButtonState(GameMemory.GameButtons + Button_Shift, VK_SHIFT);
                
                GetButtonState(GameMemory.GameButtons + Button_F1, VK_F1);
                
                
                GetButtonState(GameMemory.GameButtons + Button_F2, VK_F2);
                GetButtonState(GameMemory.GameButtons + Button_F3, VK_F3);
                
                GetButtonState(GameMemory.GameButtons + Button_LeftMouse, VK_LBUTTON);
                GetButtonState(GameMemory.GameButtons + Button_RightMouse, VK_RBUTTON);
                
#include "meta_key_functions.h"
                
                POINT CursorPos;
                GetCursorPos(&CursorPos);
                ScreenToClient(WindowHandle, &CursorPos);
                
                GameMemory.MouseX = (r32)CursorPos.x;
                GameMemory.MouseY = FrameBufferBitmap.Height - (r32)CursorPos.y;
                
                GameUpdateAndRender(&FrameBufferBitmap, &GameMemory, &OrthoRenderGroup, &RenderGroup,  TargetTimePerFrameInSeconds);
                s32 SoundDiff = 0;
                real32 SecondsSinceFlip = Win32GetSecondsElapsed(Win32GetTimeCount(),
                                                                 AudioFlipWallClock);
                
                /////////////////////////// Sound Programming ///////////////////////////////
                DWORD PlayCursor;
                DWORD WriteCursor;
                if(Global_SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
                {
                    if(!SoundIsValid)
                    {
                        //Retrieve the sound after so we don't override our sound buffer from waiting for images to load etc. Maybe could get rid of it if everything is loaded on the fly...//
                        AudioFlipWallClock = Win32GetTimeCount();
                        //
                        soundOutput.RunningSampleIndex = WriteCursor / soundOutput.BytesPerSample;
                        SoundIsValid = true;
                    }
                    
                    DWORD TargetCursor;
                    DWORD ByteToLock = (soundOutput.RunningSampleIndex * soundOutput.BytesPerSample) % soundOutput.SizeOfBuffer;
                    
                    real32 PercentofFrameTime = (TargetTimePerFrameInSeconds - SecondsSinceFlip) /
                        TargetTimePerFrameInSeconds;
                    DWORD ExpectedFrameBoundaryByte = PlayCursor + (uint32)((real32)soundOutput.BytesPerFrame * (real32)PercentofFrameTime);
                    
                    DWORD SafeWriteCursor = WriteCursor;
                    if(WriteCursor < PlayCursor)
                    {
                        SafeWriteCursor += soundOutput.SizeOfBuffer;
                    }
                    SafeWriteCursor += soundOutput.SafetyBytes;
                    bool32 AudioCardIsLowLatentcy = true;
                    AudioCardIsLowLatentcy = (SafeWriteCursor < ExpectedFrameBoundaryByte);
                    
                    if(AudioCardIsLowLatentcy)
                    {
                        TargetCursor = (ExpectedFrameBoundaryByte + soundOutput.BytesPerFrame);
                    }
                    else
                    {
                        TargetCursor = (WriteCursor + soundOutput.BytesPerFrame +
                                        soundOutput.SafetyBytes);                            
                    }
                    
                    TargetCursor %= soundOutput.SizeOfBuffer;
                    
                    DWORD BytesToWrite = 0;
                    
                    if(ByteToLock > TargetCursor){
                        
                        BytesToWrite = (soundOutput.SizeOfBuffer - ByteToLock) + TargetCursor; 
                    }
                    else {
                        BytesToWrite = TargetCursor - ByteToLock;
                    }
                    
                    //This was just for debugging puproses 3/9/17
                    if(TargetCursor >= WriteCursor) {
                        SoundDiff = TargetCursor - WriteCursor ;
                    } else {
                        SoundDiff = (TargetCursor + soundOutput.SizeOfBuffer) - WriteCursor;
                    }
                    /////
                    game_output_sound_buffer gameSoundBuffer = {};
                    gameSoundBuffer.SamplesToWrite = Align8(BytesToWrite / soundOutput.BytesPerSample);
                    gameSoundBuffer.SampleRate = soundOutput.sampleRate;
                    gameSoundBuffer.Samples = soundSamples;
                    
                    BytesToWrite = gameSoundBuffer.SamplesToWrite * soundOutput.BytesPerSample;
                    
                    GameGetSoundSamples(&GameMemory, &gameSoundBuffer);
                    
                    Win32FillSoundBuffer(&soundOutput, &gameSoundBuffer, BytesToWrite, ByteToLock);
#if HANDMADE_INTERNAL
#if 0
                    Win32_debug_cursor_position *Marker = &DebugSoundCursors[DebugCursorsIndex];
                    Marker->OutputWriteCursor = WriteCursor;
                    Marker->OutputPlayCursor = PlayCursor;
                    Marker->OutputLocation = ByteToLock;
                    Marker->OutputByteCount = BytesToWrite;
                    Marker->ExpectedFlipCursor = ExpectedFrameBoundaryByte;
                    
                    DWORD DebugUnwrappedWriteCursor = WriteCursor;
                    if(WriteCursor < PlayCursor)
                    {
                        DebugUnwrappedWriteCursor += soundOutput.SizeOfBuffer;
                    }
                    AudioLatencyBytes = DebugUnwrappedWriteCursor - PlayCursor;
                    AudioLatencySeconds = (real32)AudioLatencyBytes /
                        (real32)(soundOutput.sampleRate * soundOutput.BytesPerSample);
#endif
#endif
                } else
                {
                    SoundIsValid = false;
                }
                
                /////////////////////////////////////////////////////////////////////////////
                r32 SecondsElapsed = Win32GetSecondsElapsed(Win32GetTimeCount(), LastCounter);
#if SLEEP
                
                
                if(SecondsElapsed < TargetTimePerFrameInSeconds)
                {
                    Sleep((u32)(1000.0f*(TargetTimePerFrameInSeconds - SecondsElapsed)));
                    
                    SecondsElapsed = Win32GetSecondsElapsed(Win32GetTimeCount(), LastCounter);
                    while(SecondsElapsed < TargetTimePerFrameInSeconds)
                    {
                        SecondsElapsed = Win32GetSecondsElapsed(Win32GetTimeCount(), LastCounter);
                    }
                    
                }
                
                SecondsElapsed = Win32GetSecondsElapsed(Win32GetTimeCount(), LastCounter);
#endif
                
                win32_dimension WindowDim = Win32GetClientDimension(WindowHandle);
                
                HDC WindowDCTemp = GetDC(WindowHandle);
#if SOFTWARE_RENDERER
                RenderGroupToOutput(&RenderGroup);
                RenderGroupToOutput(&OrthoRenderGroup);
                
                Win32BltBitmapToScreen(WindowDC, WindowDim.X, WindowDim.Y);
#else
                
                
#define DRAW_FRAME_RATE 1
#if DRAW_FRAME_RATE
                SecondsElapsed = Win32GetSecondsElapsed(Win32GetTimeCount(), LastCounter);
                
                char TextBuffer[256] = {};
                Print(TextBuffer, "%f\n", SecondsElapsed);
                
                if(GameMemory.DebugFont) {
                    draw_text_options Opt = InitDrawOptions();
                    Opt.AdvanceYAtStart = true;
                    Opt.LineAdvanceModifier = 1;
                    TextToOutput(&OrthoRenderGroup, GameMemory.DebugFont, TextBuffer, 0, 0, Rect2(0, 0, (r32)Width, (r32)Height), V4(1, 1, 1, 1), &Opt);
                }
#endif
                OpenGlRenderToOutput(&RenderGroup, true);
                OpenGlRenderToOutput(&OrthoRenderGroup, true);
                
                
                LastCounter = Win32GetTimeCount();
                
                SwapBuffers(WindowDC);
#endif
                ReleaseDC(WindowHandle, WindowDCTemp);
                
                AudioFlipWallClock = Win32GetTimeCount();
                
                FirstTimeInGameLoop = false;
            }
        }
    }
}


/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */
#include <windows.h>
#include <stdio.h>
#include "calm_platform.h"
#include "calm_win32.h"
#include "calm_game.cpp"
#include "write_font_file_win32.h"
#include "dsound.h"

global_variable b32 GlobalRunning;

global_variable offscreen_buffer GlobalOffscreenBuffer;
global_variable LARGE_INTEGER GlobalTimeFrequencyDatum;
global_variable LPDIRECTSOUNDBUFFER GlobalSoundBuffer;

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

global_variable game_button GameKeys[256];

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
            Button->IsDown = !IsUp;
            if(WasDown == IsUp) {
                Button->FrameCount++;
            }
            
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
    
    if(FileHandle)
    {
        Result.Data = FileHandle;
    }
    else
    {
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


internal void 
Win32WriteAudioSamplesToBuffer(LPDIRECTSOUNDBUFFER SecondaryBuffer, s16 *SamplesToWrite, u32 WriteCursor, u32 BytesToWrite, sound_info *SoundInfo)
{
    LPVOID AudioAddress1;
    DWORD AudioSize1;
    LPVOID AudioAddress2;
    DWORD AudioSize2;
    
    if(SUCCEEDED(SecondaryBuffer->Lock(WriteCursor, BytesToWrite,
                                       &AudioAddress1, &AudioSize1,
                                       &AudioAddress2, &AudioSize2, 0)))
    {
        s16 *Address1 = (s16 *)AudioAddress1;
        for(u32 Bytes = 0;
            Bytes < AudioSize1;
            Bytes += SoundInfo->BytesPerSampleForBothChannels)
        {
            
            *Address1++ = *SamplesToWrite++;
            *Address1++ = *SamplesToWrite++;
        }
        
        s16 *Address2 = (s16 *)AudioAddress2;
        for(u32 Bytes = 0;
            Bytes < AudioSize2;
            Bytes += SoundInfo->BytesPerSampleForBothChannels)
        {
            *Address2++ = *SamplesToWrite++;
            *Address2++ = *SamplesToWrite++;
        }
    }
    
    SecondaryBuffer->Unlock(AudioAddress1, AudioSize1, AudioAddress2, AudioSize2);
}            


internal void
InitDirectSound(HWND WindowHandle, sound_info *SoundInfo, LPDIRECTSOUNDBUFFER *SecondaryBuffer)
{
    
    HMODULE DirectSoundLibrary = LoadLibrary("dsound.dll");
    if(DirectSoundLibrary)
    {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DirectSoundLibrary, "DirectSoundCreate");
        
        FreeLibrary(DirectSoundLibrary);
        
        if(DirectSoundCreate)
        {
            LPDIRECTSOUND DirectSoundObject;
            if(SUCCEEDED(DirectSoundCreate(0, &DirectSoundObject, 0)))
            {
                
                if(SUCCEEDED(DirectSoundObject->SetCooperativeLevel(WindowHandle, DSSCL_PRIORITY)))
                {
                    
                    WAVEFORMATEX BufferFormat = {};
                    BufferFormat.wFormatTag = WAVE_FORMAT_PCM;
                    BufferFormat.nChannels = (WORD)SoundInfo->NumberOfChannels;
                    BufferFormat.wBitsPerSample = (WORD)SoundInfo->BitsPerSample;
                    BufferFormat.nBlockAlign = (WORD)SoundInfo->BytesPerSampleForBothChannels;
                    BufferFormat.nSamplesPerSec = SoundInfo->SamplesPerSec;
                    BufferFormat.nAvgBytesPerSec = SoundInfo->BytesPerSec;
                    
                    DSBUFFERDESC PrimaryBufferDesc = {};
                    PrimaryBufferDesc.dwSize = sizeof(DSBUFFERDESC);
                    //NOTE(oliver): try this DSBCAPS_GLOBALFOCUS
                    PrimaryBufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
                    
                    LPDIRECTSOUNDBUFFER PrimaryBuffer;
                    if(SUCCEEDED(DirectSoundObject->CreateSoundBuffer(&PrimaryBufferDesc, &PrimaryBuffer, 0)))
                    {
                        if(SUCCEEDED(PrimaryBuffer->SetFormat(&BufferFormat)))
                        {
                            //NOTE(oliver): Format set ->  Only required on older machines
                            //              -> the kernel mixer will normally do this implicitly
                        }
                        
                    }
                    DSBUFFERDESC SecondaryBufferDesc = {};
                    SecondaryBufferDesc.dwSize = sizeof(DSBUFFERDESC);
                    SecondaryBufferDesc.dwFlags = 0;//DSBCAPS_GETCURRENTPOSITION2;
                    SecondaryBufferDesc.lpwfxFormat = &BufferFormat;
                    SecondaryBufferDesc.dwBufferBytes = SoundInfo->BufferLengthInBytes;
                    
                    if(SUCCEEDED(DirectSoundObject->CreateSoundBuffer(&SecondaryBufferDesc, SecondaryBuffer, 0)))
                    {
                        //NOTE(ollie): Secondary Buffer created
                    }
                }
            }            
        }
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
internal void
PushWorkOntoQueue(thread_info *Info, thread_work_function *WorkFunction, void *Data)
{
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
        
        u32 Width = 960;
        u32 Height = 540;
        
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
            GlobalRunning = true;
            
            SYSTEM_INFO SystemInfo;
            GetSystemInfo(&SystemInfo);
            
            u32 NumberOfProcessors = SystemInfo.dwNumberOfProcessors;
            
            u32 NumberOfUnusedProcessors = (NumberOfProcessors - 1); //NOTE(oliver): minus one to account for the one we are on
            
            thread_info ThreadInfo = {};
            ThreadInfo.Semaphore = CreateSemaphore(0, 0, NumberOfUnusedProcessors, 0);
            ThreadInfo.IndexToTakeFrom = ThreadInfo.IndexToAddTo = 0;
            ThreadInfo.WindowHandle = WindowHandle;
            
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
                Assert(ThreadCount < ArrayCount(Threads));
                Threads[ThreadCount++] = CreateThread(0, 0, Win32ThreadEntryPoint, &ThreadInfo, 0, 0);
            }
            
            /*
            PushWorkOntoQueue(&ThreadInfo, PrintTest, "Hello");
            PushWorkOntoQueue(&ThreadInfo, PrintTest, "Hello");
            PushWorkOntoQueue(&ThreadInfo, PrintTest, "Hello");
            PushWorkOntoQueue(&ThreadInfo, PrintTest, "Hello");
*/
            QueryPerformanceFrequency(&GlobalTimeFrequencyDatum);
            
            timeBeginPeriod(1);
            
            
            u32 TargetFramesPerSecond = 30;
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
            
            sound_info SoundInfo={};
            SoundInfo.NumberOfChannels = 2;
            SoundInfo.BitsPerSample = 16;
            SoundInfo.BytesPerSampleForBothChannels = (SoundInfo.NumberOfChannels*SoundInfo.BitsPerSample) / 8;
            SoundInfo.SamplesPerSec = 48000;
            SoundInfo.BytesPerSec = SoundInfo.BytesPerSampleForBothChannels*SoundInfo.SamplesPerSec;
            SoundInfo.BufferLengthInBytes = 2*SoundInfo.BytesPerSec; //NOTE(ollie): two seconds worth of audio
            //TODO(ollie): Does it matter if this is truncated? or should it be rounded?
            SoundInfo.BytesPerFrame = (u32)(TargetTimePerFrameInSeconds*SoundInfo.BytesPerSec);
            SoundInfo.BufferByteAt = 0;
            SoundInfo.BytesPerSingleChannelSamples = SoundInfo.BitsPerSample / 8;
            
            InitDirectSound(WindowHandle, &SoundInfo, &GlobalSoundBuffer);
            
            GlobalSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);
            
            memory_size AudioSamplesBufferSize = (memory_size)(SoundInfo.BytesPerSampleForBothChannels*SoundInfo.SamplesPerSec);
            s16 *AudioSamples = (s16 *)VirtualAlloc(0, AudioSamplesBufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            
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
            GameMemory.PlatformFileSize = Win32FileSize;
            GameMemory.PlatformBeginFile = Win32BeginFile;
            GameMemory.PlatformEndFile = Win32EndFile;
            GameMemory.GameRunningPtr = &GlobalRunning;
            GameMemory.SoundOn = false;
            
            Win32WriteFontFile();
            
            s64 LastCounter = Win32GetTimeCount();
            
            
            b32 FirstTimeInGameLoop = true;
            u32 LoopCount = 0;
            
            
            while(GlobalRunning)
            {
                ClearMemory(GameKeys, sizeof(GameKeys));
                
                MSG Message;
                while(PeekMessage(&Message, WindowHandle, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                if(GlobalSoundBuffer)
                {
                    DWORD CurrentPlayCursor;
                    DWORD CurrentWriteCursor;
                    if(SUCCEEDED(GlobalSoundBuffer->GetCurrentPosition(&CurrentPlayCursor, &CurrentWriteCursor)))
                    {
                        BreakPoint;
                    }
                    
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
                GetButtonState(GameMemory.GameButtons + Button_Space, VK_SPACE);
                GetButtonState(GameMemory.GameButtons + Button_Escape, VK_ESCAPE);
                GetButtonState(GameMemory.GameButtons + Button_Enter, VK_RETURN);
                GetButtonState(GameMemory.GameButtons + Button_Shift, VK_SHIFT);
                
                GetButtonState(GameMemory.GameButtons + Button_F1, VK_F1);
                
                
                GetButtonState(GameMemory.GameButtons + Button_F2, VK_F2);
                
                GetButtonState(GameMemory.GameButtons + Button_LeftMouse, VK_LBUTTON);
                GetButtonState(GameMemory.GameButtons + Button_RightMouse, VK_RBUTTON);
                
#include "meta_key_functions.h"
                
                POINT CursorPos;
                GetCursorPos(&CursorPos);
                ScreenToClient(WindowHandle, &CursorPos);
                
                GameMemory.MouseX = (r32)CursorPos.x;
                GameMemory.MouseY = FrameBufferBitmap.Height - (r32)CursorPos.y;
                
                GameUpdateAndRender(&FrameBufferBitmap, &GameMemory, TargetTimePerFrameInSeconds);
                
                ///////////////NOTE(ollie): Sound Programming ///////////////////////////
                if(GlobalSoundBuffer)
                {
                    s32 BytesToWrite = 0;
                    
                    DWORD PlayCursorAt;
                    DWORD WriteCursorAt;
                    if(SUCCEEDED(GlobalSoundBuffer->GetCurrentPosition(&PlayCursorAt, &WriteCursorAt)))
                    {
                        r32 SecondsElapsed = Win32GetSecondsElapsed(Win32GetTimeCount(), LastCounter);
                        r32 TimeRemaining = TargetTimePerFrameInSeconds - SecondsElapsed;
                        
                        if(TimeRemaining < 0.0f)
                        {
                            TimeRemaining = 0.0f;
                        }
                        
                        s32 BytesTillEndOfFrame = (s32)(TimeRemaining*SoundInfo.BytesPerSec);
                        BytesTillEndOfFrame %= SoundInfo.BytesPerFrame;
                        s32 ByteAtFrameFlip = (PlayCursorAt + BytesTillEndOfFrame);
                        
                        u32 SafetyMarginBytes = (u32)(0.5f*SoundInfo.BytesPerFrame);
                        
                        if(FirstTimeInGameLoop || TimeRemaining < 0.0f)
                        {
                            SoundInfo.BufferByteAt = WriteCursorAt + SafetyMarginBytes; 
                        }
                        
                        s32 CanonicalBufferByteAt = SoundInfo.BufferByteAt;
                        if(PlayCursorAt > WriteCursorAt)
                        {
                            ByteAtFrameFlip -= SoundInfo.BufferLengthInBytes;
                            if(CanonicalBufferByteAt > (s32)PlayCursorAt)
                            {
                                CanonicalBufferByteAt -= SoundInfo.BufferLengthInBytes;
                            }
                            
                        }
                        else if(CanonicalBufferByteAt < (s32)PlayCursorAt)
                        {
                            CanonicalBufferByteAt += SoundInfo.BufferLengthInBytes;
                        }
                        
                        //TODO(ollie): look into this
                        //Assert(CanonicalBufferByteAt >= (s32)WriteCursorAt);
                        
                        u32 SafeWriteCursor = (WriteCursorAt + SafetyMarginBytes);
                        
                        s32 Error = (ByteAtFrameFlip - CanonicalBufferByteAt);
                        
                        if(ByteAtFrameFlip > (s32)SafeWriteCursor)
                        {
                            BytesToWrite = SoundInfo.BytesPerFrame + Error + SafetyMarginBytes;
                            
                        }
                        else
                        {
                            s32 BytesPastFrameFlip = (SafeWriteCursor - ByteAtFrameFlip);
                            Assert(BytesPastFrameFlip >= 0);
                            BytesToWrite = SoundInfo.BytesPerFrame + BytesPastFrameFlip + Error;
                        }
                        
                    }
                    else
                    {
                        Assert(!"Didn't retrieve play cursor info");
                    }
                    
                    //Assert(BytesToWrite >= 0);
                    //TODO(): Why does Bytes to write go negative
                    if(BytesToWrite < 0)
                    {
                        BytesToWrite = 0;
                    }
                    
                    
                    game_sound_buffer GameSoundBuffer = {};
                    GameSoundBuffer.Samples = AudioSamples;
                    GameSoundBuffer.SamplesToWrite = BytesToWrite / SoundInfo.BytesPerSampleForBothChannels;
                    GameSoundBuffer.SamplesPerSecond = SoundInfo.SamplesPerSec;
                    GameSoundBuffer.ChannelCount = SoundInfo.NumberOfChannels;
                    
                    
                    GameWriteAudioSamples(&GameMemory, &GameSoundBuffer);
                    Win32WriteAudioSamplesToBuffer(GlobalSoundBuffer, AudioSamples,
                                                   SoundInfo.BufferByteAt,
                                                   BytesToWrite, &SoundInfo);
                    SoundInfo.BufferByteAt += BytesToWrite;
                    SoundInfo.BufferByteAt %= SoundInfo.BufferLengthInBytes;
                    
                }
                
                /////////////////////////////////////////////////////////////////////////////
                
                r32 SecondsElapsed = Win32GetSecondsElapsed(Win32GetTimeCount(), LastCounter);
                
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
                
                LastCounter = Win32GetTimeCount();
                
#if 0
                char TextBuffer[256] = {};
                sprintf_s(TextBuffer, sizeof(TextBuffer), "%f\n", SecondsElapsed);
                OutputDebugString(TextBuffer);
#endif
                
                win32_dimension WindowDim = Win32GetClientDimension(WindowHandle);
                
                HDC WindowDC = GetDC(WindowHandle);
                Win32BltBitmapToScreen(WindowDC, WindowDim.X, WindowDim.Y);
                ReleaseDC(WindowHandle, WindowDC);
                
                FirstTimeInGameLoop = false;
                LoopCount++;
            }
        }
    }
}


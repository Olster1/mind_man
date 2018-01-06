/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#define SWAP_BUFFER_INTERVAL 1

#include <stdio.h>

#define SDL_MAIN_HANDLED 1
#include "SDL2/SDL.h"

#if WIN32_BUILD
#include "SDL2/SDL_opengl.h"
#else 
#include <OpenGL/gl.h>
#include <OpenGL/gl3.h>
#include <OpenGL/glu.h>
#endif

#include "calm_platform.h"
#include "calm_sdl.h"
#include "calm_render.h"
#include "calm_font.h"

global_variable b32 GlobalRunning;
global_variable u64 GlobalTimeFrequencyDatum;
global_variable GLuint GlobalOpenGlDefaultInternalTextureFormat;

#include "calm_opengl.cpp"
#include "calm_game.cpp"

inline void UpdateButton(game_button *Button, b32 IsUp, b32 WasDown) {
    Button->IsDown = !IsUp;
    if(WasDown == IsUp) {
        Button->FrameCount++;
    }
} 

global_variable game_button GameKeys[256 + 4]; // + four extra places for keyup, down, left, right etc.

inline s64
SDLGetTimeCount()
{
    u64 Time = SDL_GetPerformanceCounter();
    return Time;
}

inline r32
SDLGetSecondsElapsed(s64 CurrentCount, s64 LastCount)
{
    
    s64 Difference = CurrentCount - LastCount;
    r32 Seconds = (r32)Difference / (r32)GlobalTimeFrequencyDatum;
    
    return Seconds;
    
}

size_t PlatformGetFileSize(SDL_RWops *FileHandle) {
    size_t Result = SDL_RWseek(FileHandle, 0, RW_SEEK_END);
    if(Result < 0) {
        Assert(!"Seek Error");
    }
    if(SDL_RWseek(FileHandle, 0, RW_SEEK_SET) < 0) {
        Assert(!"Seek Error");
    }
    return Result;
}

PLATFORM_BEGIN_FILE(Win32BeginFile)
{
    game_file_handle Result = {};
    
    SDL_RWops* FileHandle = SDL_RWFromFile(FileName, "r+");
    
    if(FileHandle)
    {
        Result.Data = FileHandle;
    }
    else
    {
        Result.HasErrors = true;
        printf("%s\n", SDL_GetError());
    }
    
    return Result;
}

PLATFORM_BEGIN_FILE_WRITE(Win32BeginFileWrite)
{
    game_file_handle Result = {};
    
    SDL_RWops* FileHandle = SDL_RWFromFile(FileName, "w+");
    
    if(FileHandle)
    {
        Result.Data = FileHandle;
    }
    else
    {
        const char* Error = SDL_GetError();
        Result.HasErrors = true;
    }
    
    return Result;
}

PLATFORM_END_FILE(Win32EndFile)
{
    SDL_RWops*  FileHandle = (SDL_RWops* )Handle.Data;
    if(FileHandle) {
        SDL_RWclose(FileHandle);
    }
}

PLATFORM_WRITE_FILE(Win32WriteFile)
{
    Handle->HasErrors = false;
    SDL_RWops *FileHandle = (SDL_RWops *)Handle->Data;
    if(!Handle->HasErrors)
    {
        Handle->HasErrors = true; 
        
        if(FileHandle)
        {
            
            if(SDL_RWseek(FileHandle, Offset, RW_SEEK_SET) >= 0)
            {
                if(SDL_RWwrite(FileHandle, Memory, 1, Size) == Size)
                {
                    Handle->HasErrors = false;
                }
                else
                {
                    Assert(!"write file did not succeed");
                }
            }
        }
    }    
}

PLATFORM_READ_FILE(Win32ReadFile)
{
    file_read_result Result = {};
    
    SDL_RWops* FileHandle = (SDL_RWops*)Handle.Data;
    if(!Handle.HasErrors)
    {
        if(FileHandle)
        {
            if(SDL_RWseek(FileHandle, Offset, RW_SEEK_SET) >= 0)
            {
                if(SDL_RWread(FileHandle, Memory, 1, Size) == Size)
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
    
    SDL_RWops* FileHandle = SDL_RWFromFile(FileName, "rb");
    
    if(FileHandle)
    {
        Result = PlatformGetFileSize(FileHandle);
        SDL_RWclose(FileHandle);
    }
    
    return Result;
}

PLATFORM_READ_ENTIRE_FILE(Win32ReadEntireFile)
{
    file_read_result Result = {};
    
    SDL_RWops* FileHandle = SDL_RWFromFile(FileName, "r+");
    
    if(FileHandle)
    {
        Result.Size = (u32)PlatformGetFileSize(FileHandle);
        Result.Data = malloc(Result.Size);
        size_t ReturnSize = SDL_RWread(FileHandle, Result.Data, 1, Result.Size);
        if(ReturnSize == Result.Size)
        {
            //NOTE(Oliver): Successfully read
        } else {
            Assert(!"Couldn't read file");
            free(Result.Data);
        }
    } else {
        const char *Error = SDL_GetError();
        Assert(!"Couldn't open file");
        
        printf("%s\n", Error);
    }
    
    SDL_RWclose(FileHandle);
    
    return Result;
}

internal SDL_GLContext
PlatformInitOpenGL(SDL_Window *WindowHandle) {
    SDL_GLContext Result = {};
    
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
    SDL_GLContext RenderContext = SDL_GL_CreateContext( WindowHandle );

    if(RenderContext) {
        if(SDL_GL_MakeCurrent(WindowHandle, RenderContext) >= 0) {
            
            b32 IsModernContext = true;
            OpenGlInit(IsModernContext);
            
            if( SDL_GL_SetSwapInterval(SWAP_BUFFER_INTERVAL) >= 0 ) {
                Result = RenderContext;
            } else {
                InvalidCodePath;
            }
        } else {
            printf("Failed to make Context current: %s", SDL_GetError());
            InvalidCodePath;
        }
    } else {
        printf("Failed to create Context: %s", SDL_GetError());
        InvalidCodePath;
    }
    
    return Result;
}

internal void
SDLFillSoundBuffer(game_output_sound_buffer *SourceBuffer, s32 BytesToWrite){
    SDL_QueueAudio(1, SourceBuffer->Samples, BytesToWrite);
}

internal void
initAudio(uint32 SamplesPerSecond, uint32 sizeOfBuffer) {
    
    SDL_AudioSpec AudioSettings = {};
    
    AudioSettings.freq = SamplesPerSecond;
    AudioSettings.format = AUDIO_S16LSB;
    AudioSettings.channels = 2;
    AudioSettings.samples = (s16)sizeOfBuffer;
    AudioSettings.callback = 0;
    
    SDL_OpenAudio(&AudioSettings, 0);
    if (AudioSettings.format != AUDIO_S16LSB) {
        printf("Failed to open audio: %s", SDL_GetError());
    }
}

void updateButtonState(game_button *Button, b32 wasReleased, b32 wasPressed, b32 repeat) {
    if (wasReleased) {
        Button->FrameCount++;
        Button->IsDown = false;
    } else if (wasPressed) {
        Button->IsDown = true;
        if(repeat) {
            
        } else {
            Button->FrameCount++;
        }
    }
}

#include <stdarg.h>
inline void SDLGetButtonState(game_button *Button, u32 NumberOfKeys, SDL_Event *Event, ...) {
    SDL_Keycode KeyCode = Event->key.keysym.sym;
    
    va_list KeyFlags;
    va_start(KeyFlags, NumberOfKeys);
    fori_count(NumberOfKeys) {
        SDL_Keycode TestCode = va_arg(KeyFlags, u32);
        if(KeyCode == TestCode) {
            b32 WasDown = false;
            b32 wasReleased = Event->key.state == SDL_RELEASED;
            b32 wasPressed = Event->key.state == SDL_PRESSED;
            
            updateButtonState(Button, wasReleased, wasPressed, Event->key.repeat);
        }
    }
    va_end(KeyFlags);
}

//TODO(ollie): Make safe for threads other than the main thread to add stuff
PLATFORM_PUSH_WORK_ONTO_QUEUE(Win32PushWorkOntoQueue) { //NOT THREAD SAFE. OTHER THREADS CAN'T ADD TO QUEUE
    for(;;)
    {
        s32 OnePastTheHead = (Info->IndexToAddTo.value + 1) % ArrayCount(Info->WorkQueue);
        if(Info->WorkQueue[Info->IndexToAddTo.value].Finished && OnePastTheHead != Info->IndexToTakeFrom.value)
        {
            thread_work *Work = Info->WorkQueue + Info->IndexToAddTo.value; 
            Work->FunctionPtr = WorkFunction;
            Work->Data = Data;
            Work->Finished = false;
            
            MemoryBarrier();
            _ReadWriteBarrier();
            
            ++Info->IndexToAddTo.value %= ArrayCount(Info->WorkQueue);
            
            MemoryBarrier();
            _ReadWriteBarrier();
            
            SDL_SemPost(Info->Semaphore);
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
    
    s32 OldValue = Info->IndexToTakeFrom.value;
    if(OldValue != Info->IndexToAddTo.value)
    {
        u32 NewValue = (OldValue + 1) % ArrayCount(Info->WorkQueue);
        
        if(SDL_AtomicCAS(&Info->IndexToTakeFrom, OldValue, NewValue) == SDL_TRUE)
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

int PlatformThreadEntryPoint(void *Info_) {
    thread_info *Info = (thread_info *)Info_;
    
    Assert(Info->WindowHandle);
    Assert(Info->ContextForThread);
    if(SDL_GL_MakeCurrent(Info->WindowHandle, Info->ContextForThread) >= 0) {
        printf("%s\n", "Success");
        //Success!!
    } else {
        const char *Error = SDL_GetError();
        //printf("Failed to make Context current: %s", );
        
        InvalidCodePath;
    }
    
    for(;;) {
        Win32DoThreadWork(Info);
        SDL_SemWait(Info->Semaphore);
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

int main(int argCount, char *args[]) {
    
    if( SDL_Init( SDL_INIT_EVERYTHING ) >= 0 ) {
#define FULL_SCREEN 0
#if FULL_SCREEN
        u32 Width = 1920;
        u32 Height = 1080;
#else 
        u32 Width = 960;
        u32 Height = 540;
#endif
        //Use OpenGL 2.1
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
        //SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
        
        
        //Create window
        SDL_Window *WindowHandle = SDL_CreateWindow( "SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, Width, Height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
        
        if(WindowHandle) {
#if FULL_SCREEN
            SDL_SetWindowPosition(WindowHandle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
#endif
            GlobalRunning = true;
            
            SDL_ShowCursor(SDL_DISABLE);
            
            SDL_GLContext MainRenderContext = PlatformInitOpenGL(WindowHandle);
            
            u32 NumberOfProcessors = SDL_GetCPUCount();
            
            u32 NumberOfUnusedProcessors = (NumberOfProcessors - 1); //NOTE(oliver): minus one to account for the one we are on
            
            thread_info ThreadInfo = {};
            ThreadInfo.Semaphore = SDL_CreateSemaphore(0);
            ThreadInfo.IndexToTakeFrom = ThreadInfo.IndexToAddTo = {};
            ThreadInfo.WindowHandle = WindowHandle;
            
            for(u32 WorkIndex = 0;
                WorkIndex < ArrayCount(ThreadInfo.WorkQueue);
                ++WorkIndex)
            {
                ThreadInfo.WorkQueue[WorkIndex].Finished = true;
            }
            
            SDL_Thread *Threads[12];
            u32 ThreadCount = 0;
            
            s32 CoreCount = Min(NumberOfUnusedProcessors, ArrayCount(Threads));
            SDL_GLContext ctx = SDL_GL_GetCurrentContext();

            for(u32 CoreIndex = 0;
                CoreIndex < (u32)CoreCount;
                ++CoreIndex)
            {
                SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
                SDL_GLContext ContextForThisThread = SDL_GL_CreateContext(WindowHandle);
                
                if(SDL_GL_MakeCurrent(WindowHandle, MainRenderContext) < 0) {
                    InvalidCodePath;
                }
                ThreadInfo.ContextForThread = ContextForThisThread;
                Assert(ThreadCount < ArrayCount(Threads));
                Threads[ThreadCount++] = SDL_CreateThread(PlatformThreadEntryPoint, "", &ThreadInfo);
            }
            SDL_GLContext ctx1 = SDL_GL_GetCurrentContext();

            if(ctx1 != ctx) {
                InvalidCodePath;
            }
        
            GlobalTimeFrequencyDatum = SDL_GetPerformanceFrequency();
            
            ///////Render Groups we pass to the game layer/////
            render_group RenderGroup = {};
            render_group OrthoRenderGroup = {};
            
            u32 TargetFramesPerSecond = 60 / SWAP_BUFFER_INTERVAL;
            r32 TargetTimePerFrameInSeconds = 1.0f / (r32)TargetFramesPerSecond;
            
            u32 BufferMemorySize = Width * Height * sizeof(u32);
            
            bitmap FrameBufferBitmap;
            FrameBufferBitmap.Width = Width;
            FrameBufferBitmap.Height = Height;
            FrameBufferBitmap.Pitch = Width*sizeof(u32);
            FrameBufferBitmap.Bits = malloc(BufferMemorySize);
            
            //NOTE(ollie) settings for sound output
            PlatformSoundOutput soundOutput = {};
            soundOutput.sampleRate = 48000;
            soundOutput.BytesPerSample = 4;
            soundOutput.SizeOfBuffer = Align8(soundOutput.sampleRate) * soundOutput.BytesPerSample*2;
            soundOutput.BytesPerFrame = (uint32)(soundOutput.sampleRate * TargetTimePerFrameInSeconds) * soundOutput.BytesPerSample;
            soundOutput.SafetyBytes = 2*(soundOutput.BytesPerFrame);
            
            int16 *soundSamples = (int16 *)malloc(soundOutput.SizeOfBuffer);
            memset(soundSamples, 0, soundOutput.SizeOfBuffer);
            
            initAudio(soundOutput.sampleRate, soundOutput.SizeOfBuffer);
            
            SDL_PauseAudio(0); //play audio
            
            game_memory GameMemory = {};
            GameMemory.GameStorageSize = GigaBytes(1);
            GameMemory.GameKeys = GameKeys;
            GameMemory.SizeOfGameKeys = ArrayCount(GameKeys);
            
            u32 TotalStorageSize = GameMemory.GameStorageSize;
            
            GameMemory.GameStorage = malloc(TotalStorageSize);
            
            memset(GameMemory.GameStorage, 0, TotalStorageSize);
            
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
            
            s64 LastCounter = SDLGetTimeCount();
            
            b32 SoundIsValid = false;
            b32 FirstTimeInGameLoop = true;
            s64 AudioFlipWallClock = SDLGetTimeCount();
            
            v2i CursorPos = {};
            
            while(GlobalRunning)
            {
                ClearMemory(GameKeys, sizeof(GameKeys));
                
                for(u32 Index = 0;
                    Index < ArrayCount(GameMemory.GameButtons);
                    Index++) {
                    GameMemory.GameButtons[Index].FrameCount = 0;                   
                }
                
                
                SDL_Event Event;
                while( SDL_PollEvent( &Event ) != 0 )
                {
                    //User requests quit
                    if( Event.type == SDL_QUIT )
                    {
                        GlobalRunning = false;
                    }
                    //Handle keypress with current mouse position
                    else if(Event.type == SDL_MOUSEBUTTONDOWN || Event.type == SDL_MOUSEBUTTONUP) {
                        game_button *MouseButton = 0;
                        if(Event.button.button == SDL_BUTTON_LEFT) {
                            MouseButton = GameMemory.GameButtons + Button_LeftMouse;
                        } else if(Event.button.button == SDL_BUTTON_RIGHT) {
                            MouseButton = GameMemory.GameButtons + Button_RightMouse;
                        }
                        b32 wasPressed = (Event.type == SDL_MOUSEBUTTONDOWN);
                        b32 wasReleased = (Event.type == SDL_MOUSEBUTTONUP);
                        b32 repeat = false;//TODO inclomplete
                        if(MouseButton) {
                            updateButtonState(MouseButton, wasReleased, wasPressed, repeat);
                        }
                    }
                    else if( Event.type == SDL_MOUSEMOTION ) {
                        u32 MouseState = SDL_GetMouseState( &CursorPos.X, &CursorPos.Y);
                    } else if(Event.type == SDL_KEYDOWN || Event.type == SDL_KEYUP) {
                        
                        SDLGetButtonState(GameMemory.GameButtons + Button_Up, 2, &Event, SDLK_UP, SDLK_w);
                        SDLGetButtonState(GameMemory.GameButtons + Button_Down, 2, &Event, SDLK_DOWN, SDLK_s);
                        SDLGetButtonState(GameMemory.GameButtons + Button_Left, 2, &Event, SDLK_LEFT, SDLK_a);
                        SDLGetButtonState(GameMemory.GameButtons + Button_Right, 2, &Event, SDLK_RIGHT, SDLK_d);
                        
                        //////////////////
                        // TODO(OLIVER): Replace this using the asci array in order to get the nice lag time on key press
                        SDLGetButtonState(GameMemory.GameButtons + Button_Arrow_Left, 1, &Event, SDLK_LEFT);
                        SDLGetButtonState(GameMemory.GameButtons + Button_Arrow_Right, 1, &Event, SDLK_RIGHT);
                        ////////////////
                        SDLGetButtonState(GameMemory.GameButtons + Button_Space, 1, &Event, SDLK_SPACE);
                        SDLGetButtonState(GameMemory.GameButtons + Button_Escape, 1, &Event, SDLK_ESCAPE);
                        SDLGetButtonState(GameMemory.GameButtons + Button_Enter, 1, &Event, SDLK_RETURN);
                        
                        u32 mod = Event.key.keysym.mod;
                        if(mod & KMOD_LSHIFT) {
                            //SDLGetButtonState(GameMemory.GameButtons + Button_Shift, 1, &Event, SDLK_SHIFT);
                        } else if(mod & KMOD_CTRL) {
                            //SDLGetButtonState(GameMemory.GameButtons + Button_Control, 1, &Event, SDLK_CONTROL);    
                        } 
                        
                        SDLGetButtonState(GameMemory.GameButtons + Button_F1, 1, &Event, SDLK_F1);
                        
                        SDLGetButtonState(GameMemory.GameButtons + Button_F2, 1, &Event, SDLK_F2);
                        SDLGetButtonState(GameMemory.GameButtons + Button_F3, 1, &Event, SDLK_F3);
                        
                        SDLGetButtonState(GameMemory.GameButtons + Button_One, 1, &Event, 0x31);
                        
                        SDLGetButtonState(GameMemory.GameButtons + Button_Two, 1, &Event, 0x32);
                        
                        SDLGetButtonState(GameMemory.GameButtons + Button_Three, 1, &Event, 0x33);
                        
                        SDLGetButtonState(GameMemory.GameButtons + Button_Four, 1, &Event, 0x34);
                        
                        SDLGetButtonState(GameMemory.GameButtons + Button_LetterS, 1, &Event, 0x53);
                        
                        
                    }
                }                
                
                
                GameMemory.MouseX = (r32)CursorPos.X;
                GameMemory.MouseY = FrameBufferBitmap.Height - (r32)CursorPos.Y;
                
                GameUpdateAndRender(&FrameBufferBitmap, &GameMemory, &OrthoRenderGroup, &RenderGroup,  TargetTimePerFrameInSeconds);
                
                //real32 SecondsSinceFlip = SDLGetSecondsElapsed(SDLGetTimeCount(), AudioFlipWallClock);
                
                s32 TargetQueueBytes = (s32)(1.6*soundOutput.sampleRate * soundOutput.BytesPerSample); //MAX one second of latency
                s32 BytesToWrite = TargetQueueBytes - SDL_GetQueuedAudioSize(1);
                game_output_sound_buffer gameSoundBuffer = {};
                gameSoundBuffer.SamplesToWrite = Align8(BytesToWrite / soundOutput.BytesPerSample);
                gameSoundBuffer.SampleRate = soundOutput.sampleRate;
                gameSoundBuffer.Samples = soundSamples;
                
                if(BytesToWrite > 0) {
                    GameGetSoundSamples(&GameMemory, &gameSoundBuffer);
                    SDLFillSoundBuffer(&gameSoundBuffer, BytesToWrite);
                }
                /////////////////////////////////////////////////////////////////////////////
                r32 SecondsElapsed = SDLGetSecondsElapsed(SDLGetTimeCount(), LastCounter);
#if SLEEP
                
                
                if(SecondsElapsed < TargetTimePerFrameInSeconds)
                {
                    SDL_Delay((u32)(1000.0f*(TargetTimePerFrameInSeconds - SecondsElapsed)));
                    
                    SecondsElapsed = SDLGetSecondsElapsed(SDLGetTimeCount(), LastCounter);
                    while(SecondsElapsed < TargetTimePerFrameInSeconds)
                    {
                        SecondsElapsed = SDLGetSecondsElapsed(SDLGetTimeCount(), LastCounter);
                    }
                    
                }
                
                SecondsElapsed = SDLGetSecondsElapsed(SDLGetTimeCount(), LastCounter);
#endif
                
#define DRAW_FRAME_RATE 1
#if DRAW_FRAME_RATE
                SecondsElapsed = SDLGetSecondsElapsed(SDLGetTimeCount(), LastCounter);
                
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
                
                LastCounter = SDLGetTimeCount();
                
                SDL_GL_SwapWindow(WindowHandle);
                
                AudioFlipWallClock = SDLGetTimeCount();
                
                FirstTimeInGameLoop = false;
            }
        }
        
        SDL_DestroyWindow( WindowHandle );
        WindowHandle = NULL;
        
    }
    
    //Quit SDL subsystems
    SDL_Quit();
    
    return 0;
}


inline void InitCharBuffer(char_buffer *Buffer, u32 Count, memory_arena *Arena, memory_arena *ArenaToAllocateInFuture) {
    
    Buffer->Chars = PushArray(Arena, char, Count);
    Buffer->Count = Count;
    Buffer->WriteIndexAt = 0;
    Buffer->IndexAt = 0;
    Buffer->Arena = ArenaToAllocateInFuture;
}

internal void ClearBuffer(char_buffer *Buffer) {
    Buffer->WriteIndexAt = 0;
    Buffer->IndexAt = 0;
}

internal void 
    InitConsole(console *Console, font *Font, bitmap *Buffer, r32 CursorPeriod, memory_arena *Arena) {
    *Console = {};
    Console->OutputFont = Console->InputFont = Font;
    Console->Buffer = Buffer;
    Console->ViewMode = VIEW_CLOSE;
    Console->InputIsActive = false;
    Console->InputTimer = {0.0f, CursorPeriod};
    u32 BufferCount = 1024;
    //TODO(): The input buffer should be changed to a memory allocator later
    InitCharBuffer(&Console->Input, BufferCount, Arena, Arena);
    InitCharBuffer(&Console->Output, BufferCount, Arena, 0);
    
} 

struct tokenizer {
    char *CharAt;
    s32 Length;
};

inline b32 IsNumeric(char Character)
{
    b32 Result = (Character >= '0' && Character <= '9') || Character == '.' || Character == 'f';
    return Result;
}

inline b32 IsAlphaNumeric(char Character)
{
    b32 Result = ((Character >= 'A' && Character <= 'Z') || 
                  (Character >= 'a' && Character <= 'z') || Character == '_');
    return Result;
}

inline void EatWhiteSpace(tokenizer *Tokenizer) {
    char Char = *Tokenizer->CharAt;
    while(Tokenizer->Length > 0 && (Char == ' ' || Char == '\r' || Char == '\n')) {
        ++Tokenizer->CharAt;
        Char = *Tokenizer->CharAt;
    }
}

#define Splice(NullTerminatedSrc, Buffer) Splice_(NullTerminatedSrc, Buffer, StringLength(NullTerminatedSrc))
		                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             
//NOTE(): This Splice function allows you to _add_ and _remove_ from the buffer. To remove from the buffer you pass a negative
//SrcLength which will remove the amount designated. It currently will just block copy waht it needs to, to keep the buffer consistent
// and will allocate memory if need be. However this hasn't been implement yet. Oliver 28/4/17 

internal void Splice_(char *NullTerminatedSrc, char_buffer *Buffer, s32 SrcLength) {
    
    char* Dest = Buffer->Chars;
    s32 DestSize = Buffer->Count;
    s32 UsedUpToAt = Buffer->IndexAt;
    s32 WriteAt = Buffer->WriteIndexAt;
    memory_arena *Arena = Buffer->Arena;
    
    Assert(WriteAt >= 0);
    Assert(UsedUpToAt >= 0);
    Assert(DestSize >= 0);
    
    if(WriteAt + SrcLength < 0) { //handles removing elements 
        SrcLength = -WriteAt;
    } else if(SrcLength + UsedUpToAt > DestSize) { 
        //Allocate bigger buffer size 
        //TODO(oliver): Do nothing for now but eventually allocate some more memory for text buffer to write in 
        //InvalidCodePath;
        return;
    } 
    
    s32 BytesToMove = UsedUpToAt - WriteAt;
    
    temp_memory TempMem = MarkMemory(Arena);
    char *TempBuffer = PushArray(Arena, char, BytesToMove + 1);
    
    MemoryCopy(Dest + WriteAt, TempBuffer, BytesToMove);
    TempBuffer[BytesToMove] = 0; //null terminate string
    
    s32 BytesWritten = MemoryCopy(NullTerminatedSrc, Dest + WriteAt);
    
    Assert(BytesWritten == SrcLength || (BytesWritten == 0 && SrcLength < 0));
    
    MemoryCopy(TempBuffer, Dest + WriteAt + SrcLength);
    
    ReleaseMemory(&TempMem);
    
    Buffer->IndexAt += SrcLength;
    Buffer->WriteIndexAt += SrcLength;
    Assert(Buffer->IndexAt <= Buffer->Count);
}

// NOTE(OLIVER): The console character buffer uses a rolling buffer. This is OK for the Output but is it alright for the Input???? We will see... Oliver 13/2/17
inline void AddToBuffer_(char_buffer *Buffer, char *FormatString, u8 *Arguments) {
    
    char TextBuffer[512];
    u32 SrcLength = Print__(TextBuffer, sizeof(TextBuffer), FormatString, Arguments);
    if(Buffer->Arena) { //Is not a rolling buffer
        Splice(TextBuffer, Buffer);
    } else {
        s32 SizeRemaining = (Buffer->Count - Buffer->IndexAt);
        if(SrcLength > SizeRemaining) {
            MemoryCopy(TextBuffer, Buffer->Chars + Buffer->IndexAt, SizeRemaining);
            u32 LeftOver = SrcLength - SizeRemaining;
            MemoryCopy(TextBuffer + SizeRemaining, Buffer->Chars, LeftOver);
            Buffer->IndexAt = LeftOver;
        } else {
            MemoryCopy(TextBuffer, Buffer->Chars + Buffer->IndexAt, SrcLength);
            Buffer->IndexAt += SrcLength;
        }
        
        Assert(Buffer->IndexAt < Buffer->Count )
    }
}

inline void AddToOutBuffer(char *FormatString, ...) {
    u8 *Arguments = ((u8 *)&FormatString) + sizeof(FormatString);
    AddToBuffer_(&DebugConsole.Output, FormatString, Arguments);
    
}

inline void AddToInBuffer(char *FormatString, ...) {
    u8 *Arguments = ((u8 *)&FormatString) + sizeof(FormatString);
    AddToBuffer_(&DebugConsole.Input, FormatString, Arguments);
    
}

inline void RemoveFromInBuffer(s32 TopIndex, s32 BottomIndex) {
    char_buffer *Buffer = &DebugConsole.Input;
    s32 Max = Max(BottomIndex, TopIndex);
    s32 Min = Min(BottomIndex, TopIndex);
    s32 TempWriteCursor = Buffer->WriteIndexAt;
    Buffer->WriteIndexAt = Max;;
    Splice_("", Buffer, Max - Min);
    Buffer->WriteIndexAt = TempWriteCursor;
}

struct command {
    tokens* Tokens;
    
    u32 NameIndex; 
    u32 ArgumentIndexes[32];
    u32 ArgumentCount;
    
    char *Error;
    b32 IsValid;
    
};
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             
internal command ParseCommand(token *Tokens, u32 TokenCount) {
    command Command = {};
    
    Command.Tokens = Tokens;
    
    Command.NameIndex = 0;
    Command.IsValid = true;
    Token.ArgumentIndexes = {};
    Token.ArgumentCount = 0;
    
    if(TokenCount > 1 && Tokens[1].Type == Word) {
        Command.NameIndex = 1;
        if(TokenCount > 2 && (Tokens[2].Type == Open_Parenthesis || Tokens[2].Type == Space)) {
            if(TokenCount > 2 && Tokens[2].Type == Number) {
                Number1 = atoi(Tokens[2].String);
                if(TokenCount > 3 && Tokens[3].Type == Number) {
                    Number1 = atoi(Tokens[2].String);
                }
            }
        } else {
            Command.Error = "No command was passed";
            Command.IsValid = false;
        }
    } else {
        Command.Error = "No command was passed";
        Command.IsValid = false;
    }
}

internal void IssueCommand(char_buffer *Buffer) {
    token Tokens[256];
    u32 TokenCount = 1;
    tokenizer Tokenizer = {};
    Tokenizer.CharAt = Buffer->Chars;
    EatWhiteSpace(&Tokenizer);
    
    while(true) {
        if(Tokenizer.Length > 0) {
            Assert(ArrayCount(Tokens) < TokenCount);
            token *Token = Tokens + TokenCount++;
            Token->Type = Null;
            Token->String = Tokenizer.CharAt;;
            Token->Length = 0;
            b32 AdvanceStringByOne = true;
            
            switch(*Tokenizer.CharAt) {
                case '(': {
                    Token->Type = Open_Parenthesis;
                } break;
                case ')': {
                    Token->Type = Close_Parenthesis;
                } break;
                case ',': {
                    Token->Type = Comma;
                } break;
                case ' ': {
                    Token->Type = Space;
                    AdvanceStringByOne = false;
                    EatAllWhiteSpace(&Tokenizer);
                } break;
                default: {
                    if(IsNumeric(*Tokenizer.CharAt)) {
                        AdvanceStringByOne = false;
                        Token->Type = Number;
                        while(Tokenizer.Length > 0 && IsNumeric(*Tokenizer.CharAt)) {
                            Tokenizer.Length--;
                            Token->Length++;
                            Tokenizer.CharAt++;
                        }
                    } else if(IsAlphaNumeric(*Tokenizer.CharAt)) {
                        Token->Type = Word;
                        while(Tokenizer.Length > 0 && IsAlphaNumeric(*Tokenizer.CharAt)) {
                            Tokenizer.Length--;
                            Token->Length++;
                            Tokenizer.CharAt++;
                        }
                    } 
                } break;
            }
            
            if(AdvanceStringByOne) {
                Token->Length++;
                Tokenizer.Length--;
                Tokenizer.CharAt++;
            }
            
            if(Tokenizer.Length == 0) {
                break;
            }
            Assert(Tokenizer.Length >= 0);
        } else {
            break;
        }
    }
    
    if(TokenCount > 0) {
        token *Token = &Tokens[0];
        if(DoStringsMatch("add", Token->String, Token->Length)) {
            
            AddToOutBuffer("Command not recongized\n");
        } else {
            AddToOutBuffer("Command not recongized\n");
        }
    }
}

internal void RenderConsole(console *Console, r32 dt) {
    
    bitmap *Buffer = Console->Buffer;
    
    rect2 Bounds = Rect2(0, 0, Buffer->Width, Buffer->Height);
    
    v4 Color = V4(1, 1, 1, 1);
    
    r32 Margin = 10;
    r32 ConsoleTotalHeight;
    switch(Console->ViewMode) {
        case VIEW_CLOSE: {
            ConsoleTotalHeight = 0;
        } break;
        case VIEW_MID: {
            ConsoleTotalHeight = 0.4f*Buffer->Height;
        } break;
        case VIEW_FULL: {
            ConsoleTotalHeight = 0.8f*Buffer->Height;
        } break;
    }
    
    r32 InputConsoleHeight =  Console->InputFont->LineAdvance + Margin;
    r32 OutBufferSize = ConsoleTotalHeight - InputConsoleHeight;
    
    rect2 OutputBounds = Rect2(0, Buffer->Height - OutBufferSize, Buffer->Width, Buffer->Height);
    
    rect2 InputBounds = Rect2(0, OutputBounds.MinY - InputConsoleHeight, Buffer->Width, OutputBounds.MinY);
    
    FillRectangle(Console->Buffer, OutputBounds, V4(0.5f, 0.0f, 0.5f, 1));
    
    FillRectangle(Console->Buffer, InputBounds, V4(0.0f, 0.5f, 0.5f, 1));
    
    if(Console->ViewMode != VIEW_CLOSE) {
        v2i Cursor = V2int(0, 0);
        
        char_buffer *Out = &Console->Output;
        s32 OutSize = Out->Count - Out->IndexAt;
        
        // TODO(OLIVER): parse until new line to make it look neater with the rolling buffer!!! 
        
        v2i CursorA = TextToOutput(Console->Buffer, Console->OutputFont, Out->Chars + Out->IndexAt, Cursor.X, Cursor.Y, Bounds, V4(0, 0, 0, 1), false, 1, OutSize);
        
        v2i CursorB = TextToOutput(Console->Buffer, Console->OutputFont, Out->Chars, CursorA.X, CursorA.Y, Bounds, V4(0, 0, 0, 1), false, 1, Out->IndexAt);
        
        Cursor = ToV2i(OutputBounds.Min) + V2int(0, CursorB.Y);
        
        Cursor = TextToOutput(Console->Buffer, Console->OutputFont, Out->Chars + Out->IndexAt, Cursor.X, Cursor.Y, Bounds, V4(0, 0, 0, 1), true, -1, OutSize);
        
        Cursor = TextToOutput(Console->Buffer, Console->OutputFont, Out->Chars, Cursor.X, Cursor.Y, Bounds, V4(0, 0, 0, 1), true, -1, Out->IndexAt);
        
        Assert(Cursor.Y == OutputBounds.Min.Y);
        
        Cursor = ToV2i(InputBounds.Min);
        
        char_buffer *In = &Console->Input;
        
        //AddToOutBuffer("%d", In->IndexAt);
        TextToOutput(Console->Buffer, Console->InputFont, In->Chars, Cursor.X, Cursor.Y, Bounds, V4(0, 0, 0, 1), true, -1, In->IndexAt, false);
        
        Cursor = TextToOutput(Console->Buffer, Console->InputFont, In->Chars, Cursor.X, Cursor.Y, Bounds, V4(0, 0, 0, 1), false, -1, In->WriteIndexAt, false);
        
        
        
        //////////////Drawing & Updating Input Cursor/////////////
        char CharacterAt = In->Chars[In->WriteIndexAt];
        char CharacterAfter = (In->WriteIndexAt + 1 <= In->IndexAt) ? In->Chars[In->WriteIndexAt + 1] : 0;
        
        if(!CharacterAt) { 
            CharacterAt = 'M';
        }
        
        s32 CursorWidth =  GetCharacterAdvanceFor(Console->InputFont, CharacterAt, CharacterAfter);
        
        rect2 InputCursor = Rect2(Cursor.X, Cursor.Y, Cursor.X + CursorWidth, Cursor.Y + Console->InputFont->LineAdvance);
        
        v4 Color1 = V4(0.2, 0.8, 0.4f, 1);
        v4 Color2 = V4(0.4, 0.6, 1, 1);
        
        timer *InputTimer = &Console->InputTimer;
        
        FillRectangle(Console->Buffer, InputCursor, SineousLerp0To0(Color1, InputTimer->Value/InputTimer->Period, Color2));
        UpdateTimer(InputTimer, dt);
        
        //////////////////////////////////////////////
        
        
    }
}

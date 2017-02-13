                    internal void 
                        InitConsole(console *Console, font *Font, bitmap *Buffer) {
                        *Console = {};
                        Console->OutputFont = Console->InputFont = Font;
                        Console->Buffer = Buffer;
                        Console->Visible = false;
                        
                    } 
                    
                    // NOTE(OLIVER): The console character buffer uses a rolling buffer. This is OK for the Output but is it alright for the Input???? We will see... Oliver 13/2/17
                    inline void AddToBuffer_(char_buffer *Buffer, char *FormatString, u8 *Arguments) {
                        
                        char TextBuffer[512];
                        u32 StringLength = Print__(TextBuffer, sizeof(TextBuffer), FormatString, Arguments);
                        
                        s32 SizeRemaining = (sizeof(Buffer->Chars) - Buffer->IndexAt);
                        if(StringLength >= SizeRemaining) {
                            MemoryCopy(TextBuffer, Buffer->Chars + Buffer->IndexAt, SizeRemaining);
                            u32 LeftOver = StringLength - SizeRemaining;
                            MemoryCopy(TextBuffer + SizeRemaining, Buffer->Chars, LeftOver);
                            Buffer->IndexAt = LeftOver;
                        } else {
                            MemoryCopy(TextBuffer, Buffer->Chars + Buffer->IndexAt, StringLength);
                            Buffer->IndexAt += StringLength;
                        }
                        
                        Assert(Buffer->IndexAt < ArrayCount(Buffer->Chars))
                    }
                    
                    inline void AddToOutBuffer(char *FormatString, ...) {
                        u8 *Arguments = ((u8 *)&FormatString) + sizeof(FormatString);
                        AddToBuffer_(&DebugConsole.Output, FormatString, Arguments);
                        
                    }
                    
                    inline void AddToInBuffer(char *FormatString, ...) {
                        u8 *Arguments = ((u8 *)&FormatString) + sizeof(FormatString);
                        AddToBuffer_(&DebugConsole.Input, FormatString, Arguments);
                        
                    }
                    
                    internal void RenderConsole(console *Console) {
                        
                        bitmap *Buffer = Console->Buffer;
                        
                        rect2 Bounds = Rect2(0, 0, Buffer->Width, Buffer->Height);
                        
                        v4 Color = V4(1, 1, 1, 1);
                        
                        r32 Margin = 10;
                        
                        r32 InputBufferSize = 100;
                        rect2 OutputBounds = Rect2(0, Buffer->Height - InputBufferSize, Buffer->Width, Buffer->Height);
                        
                        rect2 InputBounds = Rect2(0, OutputBounds.MinY - Console->InputFont->LineAdvance - Margin, Buffer->Width, OutputBounds.MinY);
                        
                        FillRectangle(Console->Buffer, OutputBounds, V4(0.5f, 0.0f, 0.5f, 1));
                        
                        FillRectangle(Console->Buffer, InputBounds, V4(0.0f, 0.5f, 0.5f, 1));
                        
                        
                        v2i Cursor = V2int(0, 0);
                        
                        char_buffer *Out = &Console->Output;
                        s32 OutSize = sizeof(Out->Chars) - Out->IndexAt;
                        
                        // TODO(OLIVER): parse until new line to make it look neater with the rolling buffer!!! Oliver 13/2/17
                        
                        v2i CursorA = TextToOutput(Console->Buffer, Console->OutputFont, Out->Chars + Out->IndexAt, Cursor.X, Cursor.Y, Bounds, V4(0, 0, 0, 1), false, 1, OutSize);
                        
                        v2i CursorB = TextToOutput(Console->Buffer, Console->OutputFont, Out->Chars, CursorA.X, CursorA.Y, Bounds, V4(0, 0, 0, 1), false, 1, Out->IndexAt);
                        
                        Cursor = ToV2i(OutputBounds.Min) + V2int(0, CursorB.Y);
                        
                        Cursor = TextToOutput(Console->Buffer, Console->OutputFont, Out->Chars + Out->IndexAt, Cursor.X, Cursor.Y, Bounds, V4(0, 0, 0, 1), true, -1, OutSize);
                        
                        Cursor = TextToOutput(Console->Buffer, Console->OutputFont, Out->Chars, Cursor.X, Cursor.Y, Bounds, V4(0, 0, 0, 1), true, -1, Out->IndexAt);
                        
                        Assert(Cursor.Y == OutputBounds.Min.Y);
                        /*
                        Cursor = ToV2i(InputBounds.Min);
                        
                        
                        char_buffer *In = &Console->Input;
                        s32 InSize = sizeof(In->Chars) - In->IndexAt;
                        
                        TextToOutput(Console->Buffer, Console->InputFont, In->Chars + In->IndexAt, Cursor.X, Cursor.Y, Bounds, V4(0, 0, 0, 1), true, -1, InSize);
                        
                        TextToOutput(Console->Buffer, Console->InputFont, In->Chars, Cursor.X, Cursor.Y, Bounds, V4(0, 0, 0, 1), true, -1, In->IndexAt);
                        */
                    }

 internal void
     ToString(s32 Value, char **TextBufferPtr, u32 TextBufferSize)
 {
     char *TextBuffer = *TextBufferPtr;
     u32 BufferIndex = 0;
     
     if(Value < 0.0f)
     {
         Assert(BufferIndex < TextBufferSize);
         TextBuffer[BufferIndex++] = '-';
         
         Value = Value*-1;
         
     }
     
     u32 OrderOfMagnitude = 1;
     
     for(;;)
     {
         s32 OrderValue = Power(10, OrderOfMagnitude);
         if(OrderValue <= Value)
         {
             OrderOfMagnitude++;
         }
         else
         {
             break;
         }
     }
     
     for(s32 OrderIndex = OrderOfMagnitude; OrderIndex > 0; --OrderIndex)
     {
         s32 OrderValue = Power(10, (OrderIndex - 1));
         
         s32 Number = (Value / OrderValue);
         Number %= 10;
         
         Assert(BufferIndex < TextBufferSize);
         TextBuffer[BufferIndex++] = (char)(Number + 48);
     }
     
     *TextBufferPtr += BufferIndex;
 }
 
 inline void
     ToString(r32 Value, char **TextBufferPtr, u32 TextBufferSize, u32 *PlacesPtr = 0)
 {
     char *TextBuffer = *TextBufferPtr;
     
     ToString((s32)Value, TextBufferPtr, TextBufferSize);
     
     TextBufferSize -= (u32)(*TextBufferPtr - TextBuffer);
     
     Assert(TextBufferSize > 0);
     *(*TextBufferPtr)++ = '.';
     TextBufferSize--;
     
     Value = Abs(Value);
     Value -= (s32)Value;
     
     s32 Places = -1;
     if(PlacesPtr)
     {
         Places = *PlacesPtr;
     }
     
     b32 FirstTime = true;
     while((Value - (s32)Value) != 0.0f && Places != 0)
     {
         Value *= 10.0f;
         
         if(FirstTime && (s32)Value == 0)
         {
             Assert(TextBufferSize > 0);
             *(*TextBufferPtr)++ = '0';
             TextBufferSize--;            
         }
         
         Places -= 1;
         FirstTime = false;
     }
     
     ToString((s32)Value, TextBufferPtr, TextBufferSize);    
     
 }
 
 inline b32 
     IsNumber(char Value)
 {
     b32 Result = (Value >= '0' && Value <= '9');
     return Result;
 }
 
 inline u32
     ToValue(char Value)
 {
     u32 Result = Value - 48;
     return Result;
     
 }
 
#define NextArgument(Arguments, type) *((type *)Arguments); Arguments = Arguments + sizeof(type); 
 
 
#define Print(Array, ...) Print_(Array, sizeof(Array), ##__VA_ARGS__);
 
 internal s32
     Print__(char *TextBuffer, u32 TextBufferSize, char *FormatString, u8 *Arguments, b32 NullTerminate = true)
 {
     
     char *TextBufferAt = TextBuffer;
     char *At = FormatString;
     
     u32 Places = 0;
     u32 *PlacesPtr = 0;
     
     b32 Modifier = false;
     while(*At)
     {
         if(Modifier)
         {
             
             u32 SizeRemaining = TextBufferSize - (u32)(TextBufferAt - TextBuffer);
             
             Assert(SizeRemaining > 0);
             
             switch(*At)
             {
                 case 's':
                 {
                     char *Argument = NextArgument(Arguments, char *);
                     while(*Argument)
                     {
                         *TextBufferAt++ = *Argument++;
                     }
                 } break;
                 case 'd':
                 {
                     s32 Argument = NextArgument(Arguments, s32); 
                     ToString(Argument, &TextBufferAt, SizeRemaining);
                 } break;
                 case 'f':
                 {
                     r64 Argument = NextArgument(Arguments, r64); 
                     ToString((r32)Argument, &TextBufferAt, SizeRemaining, PlacesPtr);
                     
                     PlacesPtr = 0;
                     
                 } break;
                 default:
                 {
                     if(IsNumber(*At))
                     {
                         Places = ToValue(*At);
                         PlacesPtr = &Places;
                         Modifier = false;
                     }
                     else
                     {
                         *TextBufferAt++ = '%';
                         *TextBufferAt++ = *At;
                     }
                 }
             }
             
             Modifier = !Modifier;
         }
         else if(*At == '%')
         {
             Modifier = true;
         }
         else
         {
             *TextBufferAt++ = *At;
         }
         At++;        
     }
     
     if(NullTerminate) {
         *TextBufferAt = 0;
     }
     
     
     s32 SizeOfString = (TextBufferAt - TextBuffer);
     Assert(SizeOfString >= 0);
     return SizeOfString;
 }
 
 inline void Print_(char *TextBuffer, u32 TextBufferSize, char *FormatString, ...) {
     
     u8 *Arguments = ((u8 *)&FormatString) + sizeof(FormatString);
     
     Print__(TextBuffer, TextBufferSize, FormatString, Arguments);
 }

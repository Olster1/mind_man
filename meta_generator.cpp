/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Oliver Marsh $
   $Notice: (C) Copyright 2015 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float r32;
typedef double r64;
typedef u32 b32;


#define InvalidCodePath (*((u8 *)0) = 0);
#define Assert(Statement) if(!(Statement)) { InvalidCodePath }
#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))

#define KiloBytes(Size) (Size*1024)
#define MegaBytes(Size) (Size * 1024 * 1024)
#define GigaBytes(Size) (Size * 1024 * 1024 * 1024)


static void *ReadFileWithNullTerminator(char *FileName)
{
    
    FILE *FileHandle;
    fopen_s(&FileHandle, FileName, "rb");
    
    fseek(FileHandle, 0, SEEK_END);
    size_t FileSize = ftell(FileHandle);
    fseek(FileHandle, 0, SEEK_SET);
    void *Result = malloc(FileSize + 1);
    
    if (fread(Result, 1, FileSize, FileHandle) == FileSize)
    {
        //Read Successful
        u8 *EndOfStream = ((u8*)Result) + FileSize;
        *EndOfStream = '\0';
    }
    else
    {
        
    }
    
    fclose(FileHandle);
    
    return Result;
    
}

struct tokenizer
{
    u8 *At;
    b32 ReachedEndOfStream;
};




enum token_type
{
    Token_Null,
    Token_Unknown,
    Token_BeginParentThesis,
    Token_EndParentThesis,
    Token_Carrot,
    Token_Plus,
    Token_Dash,
    Token_ForwardSlash,
    Token_Astrix,
    Token_Number,
    Token_Boolean,
    Token_Equals,
    Token_VariableType,
    Token_VariableName,
    Token_GreaterThan,
    Token_LessThan,
    Token_ForwardArrow,
    Token_Comma,
    Token_SemiColon,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_Hash,
    
    Token_EndOfStream
    
};

struct token
{
    token_type Type;
    char *Text;
    u32 TextLength;
    
};

static void EatAllWhiteSpace(tokenizer *Tokenizer)
{
    while (*(Tokenizer->At) == ' ' || *(Tokenizer->At) == '\t'  || *(Tokenizer->At) == '\r' || *(Tokenizer->At) == '\n')
    {
        Tokenizer->At++;
    }
    
}

inline void
SetTokenText(token *Token, char *Text, u32 Length)
{
    Token->Text = Text;
    Token->TextLength = Length;
}

inline b32 IsNumber(char Character)
{
    b32 Result = (Character >= '0' && Character <= '9');
    return Result;
}

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

static b32 DoStringsMatch(char *A, u32 LengthA, char *B, u32 LengthB)
{
    b32 Result = true;
    while (*A && *B && LengthA && LengthB)
    {
        Result &= (*A == *B);
        A++;
        B++;
        LengthA--;
        LengthB--;
    }
    
    if (LengthA != LengthB)
    {
        Result = false;
    }
    return Result;
    
}

static b32 DoStringsMatch(char *A, u32 Length, char *NullTerminatedString)
{
    b32 Result = true;
    while (*A && *NullTerminatedString && Length)
    {
        Result &= (*A == *NullTerminatedString);
        A++;
        NullTerminatedString++;
        Length--;
    }
    
    if (Length != 0 || *NullTerminatedString)
    {
        Result = false;
    }
    return Result;
    
}

static token GetNextToken(tokenizer *Tokenizer)
{
    token Token = {};
    EatAllWhiteSpace(Tokenizer);
    
    u32 TextSize = 1;
    switch (*Tokenizer->At)
    {
        case '(': { Token.Type = Token_BeginParentThesis; } break;
        case ')': { Token.Type = Token_EndParentThesis; } break;
        case '^': { Token.Type = Token_Carrot; } break;
        case '+': { Token.Type = Token_Plus; } break;
        case '-': { 
            
            if (*(Tokenizer->At + 1) == '>')
            {
                Token.Type = Token_ForwardArrow;
                Tokenizer->At++;
                TextSize = 2;
            }
            else
            {
                Token.Type = Token_Dash;
            }
            
        } break;
        case '>': { Token.Type = Token_GreaterThan; } break;
        case '<': { Token.Type = Token_LessThan; } break;
        case ',': { Token.Type = Token_Comma; } break;
        case ';': { Token.Type = Token_SemiColon; } break;
        case '#': { Token.Type = Token_Hash; } break;
        case '{': { Token.Type = Token_OpenBracket; } break;
        case '}': { Token.Type = Token_CloseBracket; } break;
        case '/': { Token.Type = Token_ForwardSlash; } break;
        case '*': { Token.Type = Token_Astrix; } break;
        case '=': { Token.Type = Token_Equals; } break;
        case '\0': { 
            Token.Type = Token_EndOfStream; 
            Tokenizer->ReachedEndOfStream = true;
        } break;
        default: { Token.Type = Token_Unknown; }
        
    }
    
    if (Token.Type != Token_Unknown)
    {
        SetTokenText(&Token, (char *)Tokenizer->At, TextSize);
        Tokenizer->At++;
    }
    else
    {
        if (IsNumber(*Tokenizer->At))
        {
            Token.Type = Token_Number;
            Token.Text = (char *)Tokenizer->At;
            
            while (IsNumeric(*Tokenizer->At))
            {
                Tokenizer->At++;
            }
            
            Token.TextLength = ((u32)Tokenizer->At - (u32)Token.Text);
        }
        else if (IsAlphaNumeric(*Tokenizer->At))
        {
            
            Token.Text = (char *)Tokenizer->At;
            
            while (IsAlphaNumeric(*Tokenizer->At) || IsNumeric(*Tokenizer->At))
            {
                Tokenizer->At++;
            }
            
            Token.TextLength = ((u32)Tokenizer->At - (u32)Token.Text);
            
            if (DoStringsMatch(Token.Text, Token.TextLength, "true") ||
                DoStringsMatch(Token.Text, Token.TextLength, "false"))
            {
                Token.Type = Token_Boolean;
            }
            else if (DoStringsMatch(Token.Text, Token.TextLength, "u32") ||
                     DoStringsMatch(Token.Text, Token.TextLength, "r32") ||
                     DoStringsMatch(Token.Text, Token.TextLength, "s32") ||
                     DoStringsMatch(Token.Text, Token.TextLength, "String"))
            {
                Token.Type = Token_VariableType;
            }
            else
            {
                Token.Type = Token_VariableName;
            }
            
        }
        else
        {
            Tokenizer->At++;
        }
        
        
    }
    
    return Token;
    
}

enum tree_element_type
{
    TreeElement_Group,
    TreeElement_Variable,
    TreeElement_Root,
    
};

struct tree_element
{
    tree_element_type Type;
    
    tree_element *Parent;
    tree_element *Next;
    
    union
    {
        struct
        {
            tree_element *BeginingElement;
            tree_element *LastElement;
        };
        struct
        {
            token Token;
        };
    };
    
};

inline b32 IsEndOfLine(char Character)
{
    b32 Result = (Character == '\n' || Character == '\r');
    return Result;
}

static tree_element *
AddElement(tree_element *ParentElement, tree_element_type Type)
{
    tree_element *Element = (tree_element *)malloc(sizeof(tree_element));
    
    *Element = {};
    Element->Type = Type;
    
    Element->Parent = ParentElement;
    if (ParentElement->LastElement)
    {
        ParentElement->LastElement = ParentElement->LastElement->Next = Element;
        
    }
    else
    {
        Assert(ParentElement->BeginingElement == 0);
        ParentElement->LastElement = ParentElement->BeginingElement = Element;
        
    }
    
    return Element;
}

inline tree_element *
AddGroupElement(tree_element **ParentElement)
{
    tree_element *Result = AddElement(*ParentElement, TreeElement_Group);
    *ParentElement = Result;
    return Result;
}

inline tree_element *
AddVariableElement(token *Token, tree_element *ParentElement)
{
    tree_element *Result = AddElement(ParentElement, TreeElement_Variable);
    Result->Token = *Token;
    return Result;
}

static void
ParseKeyFunction(tokenizer *Tokenizer, token Token)
{
    char *TextStart0 = Token.Text;
    char *TextEnd0 = Token.Text + Token.TextLength;
    char *TextStart1 = TextEnd0;
    char *TextEnd1 = 0;
    char *TextStart2 = 0;
    
    Token = GetNextToken(Tokenizer);
    Assert(Token.Type == Token_BeginParentThesis);
    
    u32 CommaCount = 0;
    while(Token.Type != Token_SemiColon)
    {
        Token = GetNextToken(Tokenizer);
        
        if(Token.Type == Token_Comma)
        {
            if(CommaCount == 0)
            {
                TextEnd1 = Token.Text;
                TextStart2 = Token.Text;
            }
            
            CommaCount++;
        }
    }
    
    u32 ParameterCount = CommaCount + 48;
    char *ParameterCountAsString = (char *)(&ParameterCount);
    u32 TextLength0 = (u32)(TextEnd0 - TextStart0);
    u32 TextLength1 = (u32)(TextEnd1 - TextStart1);
    u32 TextLength2 = (u32)((char *)Tokenizer->At - TextStart2);
    printf("%.*s_%.*s, %.*s%.*s\n", TextLength0, TextStart0, TextLength1, TextStart1, 1, ParameterCountAsString, TextLength2, TextStart2);
    
}

static void ParseLine(tokenizer *Tokenizer)
{
    token LastToken = {};
    
    b32 FunctionDefinition = false;
    b32 LastTokenDefine = false;
    
    while (!IsEndOfLine(*Tokenizer->At) && !Tokenizer->ReachedEndOfStream)
    {
        token Token = GetNextToken(Tokenizer);
        
        switch (Token.Type)
        {
            case Token_Unknown:
            {
                
                
            } break;
            case Token_BeginParentThesis:
            {
                
            } break;
            case Token_EndParentThesis:
            {
            } break;
            case Token_VariableName:
            {
                
                if(DoStringsMatch(Token.Text, Token.TextLength, "GetButtonState") && !LastTokenDefine)
                {
                    ParseKeyFunction(Tokenizer, Token);
                }
                
                LastTokenDefine = (DoStringsMatch(Token.Text, Token.TextLength, "define"));
                
            } break;
            case Token_Carrot:
            case Token_Plus:
            case Token_Dash:
            case Token_ForwardSlash:
            case Token_Astrix:
            case Token_Boolean:
            case Token_VariableType:
            case Token_Equals:
            case Token_SemiColon:
            case Token_Comma:
            case Token_Hash:
            case Token_Number:
            {
                
            } break;
            case Token_EndOfStream:
            {
                
                
            } break;
            default:
            {
                //InvalidCodePath;
            }
            
        }
        
        LastToken = Token;
        
    }
    EatAllWhiteSpace(Tokenizer);
    
}

inline b32 
IsOperator(token *Token)
{
    b32 Result = (Token->Type == Token_Carrot ||
                  Token->Type == Token_Plus ||
                  Token->Type == Token_Dash ||
                  Token->Type == Token_ForwardSlash ||
                  Token->Type == Token_Equals ||
                  Token->Type == Token_Astrix);
    
    return Result;
}

inline b32
IsLiteral(token *Token)
{
    b32 Result = (Token->Type == Token_Number || 
                  Token->Type == Token_Boolean);
    return Result;
}

enum primitive_type
{
    Primitive_Unknown,
    Primitive_Float,
    Primitive_Double,
    Primitive_Int,
    Primitive_UInt,
    Primitive_Bool,
    Primitive_String,
    
};

struct primitive
{
    primitive_type Type;
    void *Data;
};


inline b32
IsFloat(token *Token)
{
    char *At = Token->Text + Token->TextLength;
    b32 Result = (*At == 'f');
    return Result;
}

inline b32
IsDouble(token *Token)
{
    char *At = Token->Text + Token->TextLength;
    b32 Result = (*At != 'f');
    
    u32 Length = Token->TextLength;
    b32 HasDecimal = false;
    
    u32 Cycles = 0;
    for (char *At = Token->Text;
         Cycles < Length && !HasDecimal;
         Cycles++)
    {
        HasDecimal = (*At++ == '.');
    }
    
    Result &= HasDecimal;
    
    return Result;
}

struct memory_arena
{
    u32 Size;
    u32 Used;
    u8 *Base;
};


inline void
ZeroMemory(void *Memory, size_t Size)
{
    u8 *At = (u8 *)Memory;
    while (Size--)
    {
        *At++ = 0;
    }
}

inline void
InitializeMemoryArena(memory_arena *Arena, u32 Size, b32 ClearMemory = false)
{
    Arena->Size = Size;
    Arena->Used = 0;
    Arena->Base = (u8 *)malloc(Size);
    
    if (ClearMemory)
    {
        ZeroMemory(Arena->Base, Size);
    }
    
}

struct memory_mark
{
    memory_arena *Arena;
    u32 SizeAt;
};

inline memory_mark
MarkMemory(memory_arena *Arena)
{
    memory_mark Result = {};
    Result.Arena = Arena;
    Result.SizeAt = Arena->Used;
    return Result;
}

inline void
ReleaseMemory(memory_mark *MemoryMark)
{
    MemoryMark->Arena->Used = MemoryMark->SizeAt;
}

inline void *
PushSize(memory_arena *Arena, u32 Size)
{
    Assert((Arena->Used + Size) < Arena->Size);
    
    void *Result = Arena->Base + Arena->Used;
    
    Arena->Used += Size;
    
    return Result;
    
}

#define PushType(Arena, type) (type *)PushSize(Arena, sizeof(type))

struct primitive_link
{
    primitive Primitive;
    token Token;
    
    primitive_link *Next;
    
};

struct function
{
    tree_element Group;
    token NameIdentifier;
    
};

struct stack
{
    memory_arena WorkSpace;
    memory_arena VariableMemory;
    
    function *Funcitons;
    
    primitive_link *Variables[4096];
};

inline u32
pow(u32 Base, s32 Exponent)
{
    Assert(Exponent >= 0);
    u32 Result = 1;
    for (s32 Index = 0; Index < Exponent; Index++)
    { 
        Result *= Base;
    }
    
    return Result;
}

static s32 
StringToNumber(char *Text, u32 Length)
{
    s32 Result = 0;
    char *At = Text;
    for (u32 Index = 0;
         Index < Length;
         Index++, At++)
    {
        int Number = *At - 48;
        Result += Number * pow(10, (Length - Index - 1));
    }
    
    return Result;
}



inline u32
GetVariableHash(token *Token, stack *Stack)
{
    u32 Hash = 0;
    
    char *At = Token->Text;
    for (u32 Index = 0; Index < Token->TextLength; Index++, At++)
    {
        Hash += (*At) * 29;
    }
    Hash %= ArrayCount(Stack->Variables);
    return Hash;
}

inline size_t
GetDataSize(primitive *Primitive)
{
    size_t Size = 0;
    switch (Primitive->Type)
    {
        case Primitive_Double:
        {
            Size = sizeof(r64);
        } break;
        case Primitive_Float:
        {
            Size = sizeof(r32);
        } break;
        case Primitive_Int:
        {
            Size = sizeof(s32);
        } break;
        case Primitive_Bool:
        {
            Size = sizeof(s32);
        } break;
        
    }
    
    return Size;
}

inline void CopyMemory(void *Source, void *Dest, size_t Size)
{
    u8 *A = (u8 *)Source;
    u8 *B = (u8 *)Dest;
    
    while (Size)
    {
        *B++ = *A++;
        Size--;
    }
}
static primitive
CopyPrimitive(memory_arena *Arena, primitive *PrimitiveToCopy)
{
    primitive Primitive = {};
    Primitive.Type = PrimitiveToCopy->Type;
    
    size_t Size = GetDataSize(PrimitiveToCopy);
    Primitive.Data = PushSize(Arena, (u32)Size);
    
    CopyMemory(PrimitiveToCopy->Data, Primitive.Data, Size);
    
    return Primitive;
    
}

static void
AddVariable(primitive *Primitive, token *Token, stack *Stack)
{
    primitive_link *NewLink = PushType(&Stack->VariableMemory, primitive_link);
    NewLink->Token = *Token;
    NewLink->Primitive = CopyPrimitive(&Stack->VariableMemory, Primitive);
    u32 Hash = GetVariableHash(Token, Stack);
    NewLink->Next = Stack->Variables[Hash];
    Stack->Variables[Hash] = NewLink;
    
}

static primitive
GetVariable(token *Token, stack *Stack, primitive *Primitive = 0)
{
    primitive Result = {};
    
    u32 Hash = GetVariableHash(Token, Stack);
    
    primitive_link *Link= Stack->Variables[Hash];
    while (Link)
    {
        if (DoStringsMatch(Token->Text, Token->TextLength, Link->Token.Text, Link->Token.TextLength))
        {
            Result = Link->Primitive;
            if (Primitive)
            {       
                if (Primitive->Type != Result.Type)
                {
                    printf("you are tyring to assign type %s to a variable of type %s", Primitive->Type, Result.Type);
                }
                else
                {
                    CopyMemory(Primitive->Data, Result.Data, GetDataSize(Primitive));
                }
            }
            break;
        }
        else
        {
            Link = Link->Next;
        }
        
    }
    
    if (!Link)
    {
        if (Primitive)
        {
            AddVariable(Primitive, Token, Stack);
        }
        else
        {
            printf("the variable you have referenced is undeclared");
        }
    }
    
    
    return Result;
}



inline primitive 
ToPrimitive(token *Token, stack *Stack)
{
    primitive Primitive = {};
    if (IsLiteral(Token))
    {
        
        switch (Token->Type)
        {
            case Token_Number:
            {
                if (IsDouble(Token))
                {
                    Primitive.Type = Primitive_Double;
                    Primitive.Data = PushSize(&Stack->WorkSpace, sizeof(r64));
                }
                else if (IsFloat(Token))
                {
                    Primitive.Type = Primitive_Float;
                    Primitive.Data = PushSize(&Stack->WorkSpace, sizeof(r32));
                }
                else
                {
                    Primitive.Type = Primitive_Int;
                    Primitive.Data = PushSize(&Stack->WorkSpace, sizeof(s32));
                    s32 *Data = (s32 *)Primitive.Data;
                    *Data = StringToNumber(Token->Text, Token->TextLength);
                }
                
                
            } break;
            case Token_Boolean:
            {
                Primitive.Type = Primitive_Bool;
                Primitive.Data = PushSize(&Stack->WorkSpace, sizeof(s32));
                
            } break;
        }
    }
    else if (Token->Type == Token_VariableName)
    {
        Primitive = GetVariable(Token, Stack);
    }
    else
    {
        printf("Oops! A literal was expected");
    }
    
    return Primitive;
}


#define EVALUTE_GROUP(Name)  static primitive Name(tree_element *ParentElement, stack *Stack)

EVALUTE_GROUP(EvaluateGroup);

inline primitive
RetrievePrimitive(tree_element *Element, stack *Stack)
{
    primitive Result;
    if (Element->Type == TreeElement_Group)
    {
        Result = EvaluateGroup(Element, Stack);
    }
    else
    {
        Result = ToPrimitive(&Element->Token, Stack);
    }
    
    return Result;
    
    
}

EVALUTE_GROUP(EvaluateGroup)
{
    Assert(ParentElement->Type == TreeElement_Group);
    
    tree_element *Element = ParentElement->BeginingElement;
    primitive Result = {};  
    
    while (Element)
    {
        tree_element *ElementA = Element;
        if (!Result.Data)
        {
            if (Element->Next && Element->Next->Token.Type != Token_Equals)
            {
                Result = RetrievePrimitive(Element, Stack);
            }
            
            Element = Element->Next;
        }
        
        if (Element)
        {
            
            primitive A = Result;
            
            tree_element *OperatorElement = Element;
            Assert(OperatorElement->Type == TreeElement_Variable);
            
            Element = Element->Next;
            
            tree_element *ElementB = Element;
            if (!ElementB)
            {
                printf("Oops! You seem to have forgot a literal after an operator.");               
            }
            else
            {
                primitive B = RetrievePrimitive(Element, Stack);
                
                
                Element = Element->Next;
                
                switch (OperatorElement->Token.Type)
                {
                    case Token_Carrot:
                    {
                        
                    } break;
                    case Token_Plus:
                    {
                        if (A.Type == Primitive_Int && B.Type == Primitive_Int)
                        {
                            Result.Type = Primitive_Int;
                            int *Data = (int *)Result.Data;
                            *Data = *((int *)A.Data) + *((int *)B.Data);
                        }
                    } break;
                    case Token_Dash:
                    {
                        
                    } break;
                    case Token_ForwardSlash:
                    {
                        if (A.Type == Primitive_Int && B.Type == Primitive_Int)
                        {
                            Result.Type = Primitive_Int;
                            int *Data = (int *)Result.Data;
                            *Data = *((int *)A.Data) / *((int *)B.Data);
                            
                        }
                    } break;
                    case Token_Astrix:
                    {
                        
                    } break;
                    case Token_Equals:
                    {
                        if (ElementA->Token.Type == Token_VariableName)
                        {
                            Assert(ElementB->Type == TreeElement_Group);
                            GetVariable(&ElementA->Token, Stack, &B);
                            
                        }
                        else
                        {
                            printf("Oops! You need to assign a value to a variable type");
                        }
                        
                    } break;
                    default:
                    {
                        printf("Oops! Operator expected");
                    }
                }
            }
            
        }
        
    }
    
    return Result;
    
    
}

int main(int argc, char* argv[])
{
    void *File = ReadFileWithNullTerminator("calm_win32.cpp");
    tokenizer Tokenizer = {};
    Tokenizer.At = (u8 *)File;
    Tokenizer.ReachedEndOfStream = false;
    
    stack Stack = {};
    InitializeMemoryArena(&Stack.WorkSpace, MegaBytes(2));
    InitializeMemoryArena(&Stack.VariableMemory, MegaBytes(2), true);
    
    
    while (Tokenizer.At && !Tokenizer.ReachedEndOfStream)
    {
        ParseLine(&Tokenizer);
    }
    
    
    return 0;
}

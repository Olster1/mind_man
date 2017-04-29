                                                                                                                                                                                        enum buffer_type {
                                                                                                                                                                                        INPUT_BUFFER,
                                                                                                                                                                                        OUTPUT_BUFFER,
                                                                                                                                                                                    };
                                                                                                                                                                                    
                                                                                                                                                                                    struct char_buffer{
                                                                                                                                                                                        s32 IndexAt;
                                                                                                                                                                                        s32 WriteIndexAt;
                                                                                                                                                                                        u32 Count;
                                                                                                                                                                                        
                                                                                                                                                                                        char *Chars;
                                                                                                                                                                                        
                                                                                                                                                                                        memory_arena *Arena; 
                                                                                                                                                                                    };
                                                                                                                                                                                    
                                                                                                                                                                                    enum view_mode {
                                                                                                                                                                                        VIEW_CLOSE,
                                                                                                                                                                                        VIEW_MID, 
                                                                                                                                                                                        VIEW_FULL,
                                                                                                                                                                                    };
                                                                                                                                                                                    
                                                                                                                                                                                    enum token_type {
                                                                                                                                                                                        Null,
                                                                                                                                                                                        Word,
                                                                                                                                                                                        Number,
                                                                                                                                                                                        Space,
                                                                                                                                                                                        Open_Parenthesis,
                                                                                                                                                                                        Close_Parenthesis,
                                                                                                                                                                                        Comma,
                                                                                                                                                                                        
                                                                                                                                                                                    };
                                                                                                                                                                                    
                                                                                                                                                                                    struct token {
                                                                                                                                                                                        token_type Type;
                                                                                                                                                                                        char *String;
                                                                                                                                                                                        s32 Length;
                                                                                                                                                                                    };
                                                                                                                                                                                    
                                                                                                                                                                                    struct console {
                                                                                                                                                                                        font *InputFont;
                                                                                                                                                                                        font *OutputFont;
                                                                                                                                                                                        
                                                                                                                                                                                        view_mode ViewMode;
                                                                                                                                                                                        
                                                                                                                                                                                        char_buffer Output;
                                                                                                                                                                                        char_buffer Input;
                                                                                                                                                                                        
                                                                                                                                                                                        timer InputTimer; 
                                                                                                                                                                                        b32 InputIsActive;
                                                                                                                                                                                        
                                                                                                                                                                                        bitmap *Buffer;
                                                                                                                                                                                        
                                                                                                                                                                                    };
                                                                                                                                                                                    
                                                                                                                                                                                    static console DebugConsole;

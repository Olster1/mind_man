                                                                                                                                                                                                                                                                                            enum buffer_type {
                                                                                                                                                                                                                                                                                            INPUT_BUFFER,
                                                                                                                                                                                                                                                                                            OUTPUT_BUFFER,
                                                                                                                                                                                                                                                                                        };
                                                                                                                                                                                                                                                                                        
                                                                                                                                                                                                                                                                                        struct char_buffer {
                                                                                                                                                                                                                                                                                            //This is for the rolling buffer
                                                                                                                                                                                                                                                                                            s32 IndexAt;
                                                                                                                                                                                                                                                                                            //
                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                            s32 WriteIndexAt;
                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                            //Size of the buffer. Change this?
                                                                                                                                                                                                                                                                                            u32 Count;
                                                                                                                                                                                                                                                                                            char *Chars;
                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                            //This is to allocate from if we need more room; for non-rolling buffers
                                                                                                                                                                                                                                                                                            memory_arena *Arena; 
                                                                                                                                                                                                                                                                                            //
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
                                                                                                                                                                                                                                                                                        
                                                                                                                                                                                                                                                                                        enum value_type {
                                                                                                                                                                                                                                                                                            VALUE_FLOAT, 
                                                                                                                                                                                                                                                                                            VALUE_BOOL,
                                                                                                                                                                                                                                                                                            VALUE_INT,
                                                                                                                                                                                                                                                                                            VALUE_STRING,
                                                                                                                                                                                                                                                                                        };
                                                                                                                                                                                                                                                                                        
                                                                                                                                                                                                                                                                                        struct value_data {
                                                                                                                                                                                                                                                                                            value_type Type;
                                                                                                                                                                                                                                                                                            s64 Value;
                                                                                                                                                                                                                                                                                        };
                                                                                                                                                                                                                                                                                        
                                                                                                                                                                                                                                                                                        struct console {
                                                                                                                                                                                                                                                                                            font *InputFont;
                                                                                                                                                                                                                                                                                            font *OutputFont;
                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                            view_mode ViewMode;
                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                            char_buffer Output;
                                                                                                                                                                                                                                                                                            char_buffer Input;
                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                            timer InputTimer; 
                                                                                                                                                                                                                                                                                            b32 InputIsActive;
                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                            b32 InputIsHot;
                                                                                                                                                                                                                                                                                            rect2 InputConsoleRect;
                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                            timer InputClickTimer; 
                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                            render_group *RenderGroup;
                                                                                                                                                                                                                                                                                            
                                                                                                                                                                                                                                                                                        };
                                                                                                                                                                                                                                                                                        
                                                                                                                                                                                                                                                                                        static console DebugConsole;

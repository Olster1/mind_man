                                                                                                enum buffer_type {
                                                                                                INPUT_BUFFER,
                                                                                                OUTPUT_BUFFER,
                                                                                            };
                                                                                            
                                                                                            struct char_buffer{
                                                                                                u32 IndexAt;
                                                                                                char Chars[1024];
                                                                                            };
                                                                                            
                                                                                            enum view_mode {
                                                                                                VIEW_CLOSE,
                                                                                                VIEW_MID, 
                                                                                                VIEW_FULL,
                                                                                            };
                                                                                            
                                                                                            
                                                                                            struct console {
                                                                                                font *InputFont;
                                                                                                font *OutputFont;
                                                                                                
                                                                                                view_mode ViewMode;
                                                                                                
                                                                                                char_buffer Output;
                                                                                                char_buffer Input;
                                                                                                
                                                                                                
                                                                                                
                                                                                                bitmap *Buffer;
                                                                                                
                                                                                            };
                                                                                            
                                                                                            static console DebugConsole;

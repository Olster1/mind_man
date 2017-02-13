                                                                                            enum buffer_type {
                                                                                            INPUT_BUFFER,
                                                                                            OUTPUT_BUFFER,
                                                                                        };
                                                                                        
                                                                                        struct char_buffer{
                                                                                            u32 IndexAt;
                                                                                            char Chars[1024];
                                                                                        };
                                                                                        
                                                                                        struct console {
                                                                                            font *InputFont;
                                                                                            font *OutputFont;
                                                                                            
                                                                                            b32 Visible; 
                                                                                            
                                                                                            char_buffer Output;
                                                                                            char_buffer Input;
                                                                                            
                                                                                            bitmap *Buffer;
                                                                                            
                                                                                        };
                                                                                        
                                                                                        static console DebugConsole;

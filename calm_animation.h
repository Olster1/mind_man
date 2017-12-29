                            struct quality_info {
                            r32 MaxValue;
                            b32 IsPeriodic;
                        };
                        
                        enum animation_qualities {
                            DIRECTION,
                            ANIMATE_QUALITY_COUNT
                        };
                        
                        struct animation {
                            bitmap Frames[16];
                            u32 FrameCount;
                            r32 Qualities[ANIMATE_QUALITY_COUNT];
                            char *Name;
                        };
                        
                        struct animation_list_item {
                            timer Timer;
                            u32 FrameIndex;
                            
                            animation *Animation;
                            
                            animation_list_item *Prev;
                            animation_list_item *Next;
                        };

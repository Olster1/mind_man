                                                        internal void UpdateMenu(game_state *GameState, game_memory *Memory, bitmap *Buffer, r32 dt) {
                                                        char *SoundString = (Memory->SoundOn) ? "Sound Off" : "Sound On";
                                                        
                                                        char *MenuOptions[3];
                                                        MenuOptions[0] = "Continue";
                                                        MenuOptions[1] = SoundString;
                                                        MenuOptions[2] = "Exit";
                                                        
                                                        
                                                        if(WasPressed(Memory->GameButtons[Button_Up])) {
                                                            if(--GameState->MenuIndex < 0) {
                                                                GameState->MenuIndex = ArrayCount(MenuOptions) - 1;
                                                            }
                                                            GameState->MenuTime = 0.0f;
                                                        }
                                                        
                                                        if(WasPressed(Memory->GameButtons[Button_Down])) {
                                                            if(++GameState->MenuIndex >= ArrayCount(MenuOptions)) {
                                                                GameState->MenuIndex = 0;
                                                            }
                                                            GameState->MenuTime = 0.0f;
                                                        }
                                                        
                                                        if(WasPressed(Memory->GameButtons[Button_Enter])) {
                                                            switch(GameState->MenuIndex) {
                                                                case 0: {
                                                                    GameState->GameMode = PLAY_MODE;
                                                                } break;
                                                                case 1: {
                                                                    Memory->SoundOn = !Memory->SoundOn;
                                                                } break;
                                                                case 2: {
                                                                    *Memory->GameRunningPtr = false;
                                                                } break;
                                                            }
                                                            GameState->MenuTime = 0.0f;
                                                        }
                                                        
                                                        r32 tValue = GameState->MenuTime/GameState->MenuPeriod;
                                                        v4 Color = SineousLerp0To0(V4(1, 0, 1, 1), tValue, V4(1, 1, 0, 1));
                                                        
                                                        GameState->MenuTime += dt;
                                                        
                                                        if((GameState->MenuTime / GameState->MenuPeriod) > 1.0f) {
                                                            GameState->MenuTime = 0.0f;
                                                        }
                                                        
                                                        s32 MenuLineAdvance = Buffer->Height / (ArrayCount(MenuOptions) + 1);
                                                        s32 YCursor = Buffer->Height - MenuLineAdvance;
                                                        s32 HalfBuffer = Buffer->Width / 2;
                                                        fori(MenuOptions) {
                                                            v4 OptionColor = (Index == GameState->MenuIndex) ? Color : V4(0, 0, 0, 1);
                                                            char *MenuOption = MenuOptions[Index];
                                                            s32 XCursor = HalfBuffer - (GetWidth(GetTextBounds(GameState->Font, MenuOption)) / 2);
                                                            TextToOutput(Buffer, GameState->Font, MenuOption, XCursor, YCursor, Rect2(0, 0, Buffer->Width, Buffer->Height), OptionColor);
                                                            YCursor -= MenuLineAdvance;
                                                            
                                                        }
}
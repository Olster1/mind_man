
internal void UpdateAndRenderMenu(menu *Menu, render_group *RenderGroup, char **MenuOptions, s32 MenuOptionsCount, r32 dt, game_memory *Memory) {
    
    if(WasPressed(Memory->GameButtons[Button_Up])) {
        if(--Menu->IndexAt < 0) {
            Menu->IndexAt = MenuOptionsCount - 1;
        }
        Menu->Timer.Value = 0.0f;
    }
    
    if(WasPressed(Memory->GameButtons[Button_Down])) {
        if(++Menu->IndexAt >= MenuOptionsCount) {
            Menu->IndexAt = 0;
        }
        Menu->Timer.Value = 0.0f;
    }
    
    r32 tValue = CanonicalValue(&Menu->Timer);
    v4 Color = SineousLerp0To0(V4(1, 0, 1, 1), tValue, V4(1, 1, 0, 1));
    
    UpdateTimer(&Menu->Timer, dt);
    
    r32 ScreenHeight = RenderGroup->ScreenDim.Y;
    r32 ScreenWidth = RenderGroup->ScreenDim.X;
    
    s32 MenuLineAdvance = (s32)(ScreenHeight / (MenuOptionsCount + 1));
    s32 YCursor = (s32)ScreenHeight - MenuLineAdvance;
    s32 HalfBuffer = (s32)(ScreenWidth / 2);
    forN_((u32)MenuOptionsCount, Index) {
        v4 OptionColor = ((s32)Index == Menu->IndexAt) ? Color : V4(0, 0, 0, 1);
        char *MenuOption = MenuOptions[Index];
        s32 XCursor = (s32)(HalfBuffer - (GetWidth(GetTextBounds(Menu->Font, MenuOption)) / 2));
        draw_text_options DrawOptions = InitDrawOptions();
        TextToOutput(RenderGroup, Menu->Font, MenuOption, (r32)XCursor, (r32)YCursor, Rect2(0, 0, ScreenWidth, ScreenHeight), OptionColor, &DrawOptions);
        YCursor -= MenuLineAdvance;
        
    }
    
}

internal void UpdateMenu(menu *Menu, game_state *GameState, game_memory *Memory, render_group *RenderGroup, r32 dt) {
    
    char *SoundString = "Sound On";
    if (Memory->SoundOn) {
        SoundString = "Sound Off";
    }
    
    char *MenuOptions[3];
    MenuOptions[0] = "Continue";
    MenuOptions[1] = SoundString;
    MenuOptions[2] = "Exit";
    
    
    if(WasPressed(Memory->GameButtons[Button_Enter])) {
        switch(Menu->IndexAt) {
            case 0: {
                GameState->GameMode = PLAY_MODE;
            } break;
            case 1: {
                Memory->SoundOn = !Memory->SoundOn;
            } break;
            case 2: {
                EndProgram(Memory);
            } break;
        }
        Menu->Timer.Value = 0.0f;
    }
    
    UpdateAndRenderMenu(Menu, RenderGroup, MenuOptions, ArrayCount(MenuOptions), dt, Memory);
    
}
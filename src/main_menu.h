#pragma once

enum {
    MMI_START = 0,
    MMI_STORY,
    MMI_HOW_TO,
    MMI_COUNT
};

enum {
    MMP_MAIN,
    MMP_STORY,
    MMP_HOW_TO,
    MMP_COUNT
};

typedef struct {
    int page;
    int index;
} main_menu;

void main_menu_draw(main_menu *mm);

void main_menu_update(main_menu *mm);

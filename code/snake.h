#ifndef SNAKE_H
#define SNAKE_H

struct MenuItem {
    const char *text;
    void (*action)();
};

struct Menu {
    MenuItem *items;
    f32 x, y;
    int item_count;
    MenuItem *selected;
};

struct Snake {
    struct Cell *cells;
    int length;
};

enum GameMode {
    Mode_Start,
    Mode_Play,
    Mode_End,
};

struct GameState {
    GameMode game_mode;
    Snake *snake;
};

struct Input {
    b32 up;
    b32 down;
    b32 left;
    b32 right;
    b32 enter;
};
   
enum Dir {
    Left = 1,
    Right,
    Up,
    Down,
};

struct Cell {
    int x;
    int y;
    Dir dir;

    Cell() {
        x = 0;
        y = 0;
        dir = (Dir)0;
    }
    
    Cell(int x, int y) {
        this->x = x;
        this->y = y;
        dir = (Dir)0;
    }

    Cell(int x, int y, Dir dir) {
        this->x = x;
        this->y = y;
        this->dir = dir;
    }
};

struct QuadV {
    f32 x, y;
    f32 u, v;
};

#endif // SNAKE_H

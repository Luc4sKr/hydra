#ifndef EDITOR_H
#define EDITOR_H

#include <termio.h>

struct editorConfig {
    int cx;
    int cy;
    int screen_rows;
    int screen_cols;
    struct termios orig_termios;
};

struct editorConfig e_config;

// append buffer
struct abuff {
    char* b;
    int len;
};

enum editorKey {
    // valores grandes para não gerarem conflito com
    // nenhum pressionamento de tecla comum
    ARROW_UP = 1000,
    ARROW_LEFT = 1001,
    ARROW_DOWN = 1002,
    ARROW_RIGHT = 1003
};

#endif
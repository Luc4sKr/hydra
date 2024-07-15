#ifndef EDITOR_H
#define EDITOR_H

#include <termio.h>

typedef struct editor_row {
    int size;
    char *chars;
} editor_row;

struct editorConfig {
    int cx;
    int cy;
    int rowoff;
    int screenrows;
    int screencols;
    int numrows;
    editor_row* row;
    struct termios orig_termios;
};

extern struct editorConfig e_config;

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
    ARROW_RIGHT = 1003,

    PAGE_UP = 1004,
    PAGE_DOWN = 1005,

    HOME_KEY = 1006,
    END_KEY = 1007,

    DEL_KEY = 1008,
};

void initEditor();
void editorScroll();
void editorAppendRows(char* s, size_t len);

#endif
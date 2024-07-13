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

#endif
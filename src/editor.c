#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "editor.h"
#include "terminal.h"

struct editorConfig e_config;

void initEditor() {
    e_config.cx = 0;
    e_config.cy = 0;
    e_config.rowoff = 0;
    e_config.coloff = 0;
    e_config.numrows = 0;
    e_config.row = NULL;

    if (getWindowSize(&e_config.screenrows, &e_config.screencols) == -1)
        die("getWindowSize");
}

void editorScroll() {
    if (e_config.cy < e_config.rowoff) {
        e_config.rowoff = e_config.cy;
    }

    if (e_config.cy >= e_config.rowoff + e_config.screenrows) {
        e_config.rowoff = e_config.cy - e_config.screenrows + 1;
    }

    if (e_config.cx < e_config.coloff) {
        e_config.coloff = e_config.cx;
    }

    if (e_config.cx >= e_config.coloff + e_config.screencols) {
        e_config.coloff = e_config.cx - e_config.screencols + 1;
    }
}

void editorAppendRows(char* s, size_t len) {
    e_config.row = realloc(e_config.row, sizeof(editor_row) * (e_config.numrows + 1));

    int at = e_config.numrows;
    e_config.row[at].size = len;
    e_config.row[at].chars = malloc(len + 1);
    memcpy(e_config.row[at].chars, s, len);
    e_config.row[at].chars[len] = '\0';
    e_config.numrows++;
}
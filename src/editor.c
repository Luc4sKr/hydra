#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>

#include "editor.h"
#include "terminal.h"
#include "config.h"

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

void editorUpdateRow(editor_row* row) {
    int tabs = 0;

    for (int j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            tabs++;
        }
    }

    free(row->render);
    row->render = malloc(row->size + tabs * (HYDRA_TAB_STOP - 1) + 1);

    int idx = 0;
    for (int j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % HYDRA_TAB_STOP != 0) {
                row->render[idx++] = ' ';
            }
        } else {
            row->render[idx++] = row->chars[j];
        }
    }

    row->render[idx] = '\0';
    row->rsize = idx;
}


void editorAppendRows(char* s, size_t len) {
    e_config.row = realloc(e_config.row, sizeof(editor_row) * (e_config.numrows + 1));

    int at = e_config.numrows;
    e_config.row[at].size = len;
    e_config.row[at].chars = malloc(len + 1);
    memcpy(e_config.row[at].chars, s, len);
    
    e_config.row[at].chars[len] = '\0';
    e_config.row[at].rsize = 0;
    e_config.row[at].render = NULL;
    editorUpdateRow(&e_config.row[at]);

    e_config.numrows++;
}

void editorMoveCursor(int key) {
    editor_row* row = (e_config.cy >= e_config.numrows) ? NULL : &e_config.row[e_config.cy];

    switch (key) {
        case ARROW_UP:
            if (e_config.cy != 0) {
                e_config.cy--;
            }
            break;
        case ARROW_LEFT:
            if (e_config.cx != 0) {
                e_config.cx--;
            } else if (e_config.cy > 0) {
                e_config.cy--;
                e_config.cx = e_config.row[e_config.cy].size;
            }
            break;
        case ARROW_DOWN:
            if (e_config.cy < e_config.numrows) {
                e_config.cy++;
            }
            break;
        case ARROW_RIGHT:
            if (row && e_config.cx < row->size) {
                e_config.cx++;
            } else if (row && e_config.cx == row->size) {
                e_config.cy++;
                e_config.cx = 0;
            }
            break;
    }

    // move o cursor para o fim da linha quando cx > que a linha
    row = (e_config.cy >= e_config.numrows) ? NULL : &e_config.row[e_config.cy];
    int rowlen = row ? row->size : 0;

    if (e_config.cx > rowlen) {
        e_config.cx = rowlen;
    }
}

void editorProcessKeypress() {
    int c = editorReadKey();

    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case PAGE_UP:
        case PAGE_DOWN:
        {
            int times = e_config.screenrows;
            while (times--) {
                editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
        }
        break;

        case HOME_KEY:
            e_config.cx = 0;
            break;
        case END_KEY:
            e_config.cx = e_config.screencols - 1;
            break;

        case ARROW_UP:
        case ARROW_LEFT:
        case ARROW_DOWN:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
    }
}
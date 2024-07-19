#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>

#include "editor.h"
#include "terminal.h"
#include "abuff.h"
#include "config.h"

struct editorConfig e_config;

void initEditor() {
    e_config.cx = 0;
    e_config.cy = 0;
    e_config.rx = 0;
    e_config.rowoff = 0;
    e_config.coloff = 0;
    e_config.numrows = 0;
    e_config.row = NULL;
    e_config.filename = NULL;

    if (getWindowSize(&e_config.screenrows, &e_config.screencols) == -1) {
        die("getWindowSize");
    }

    e_config.screenrows -= 1;
}

int editorRowCxToRx(editor_row* row, int cx) {
    int rx = 0;

    for (int i = 0; i < cx; i++) {
        if (row->chars[i] == '\t') {
            rx += (HYDRA_TAB_STOP - 1) - (rx % HYDRA_TAB_STOP);
        }

        rx++;
    }

    return rx;
}

void editorScroll() {
    e_config.rx = 0;

    if (e_config.cy < e_config.numrows) {
        e_config.rx = editorRowCxToRx(&e_config.row[e_config.cy], e_config.cx);
    }

    if (e_config.cy < e_config.rowoff) {
        e_config.rowoff = e_config.cy;
    }

    if (e_config.cy >= e_config.rowoff + e_config.screenrows) {
        e_config.rowoff = e_config.cy - e_config.screenrows + 1;
    }

    if (e_config.rx < e_config.coloff) {
        e_config.coloff = e_config.rx;
    }

    if (e_config.rx >= e_config.coloff + e_config.screencols) {
        e_config.coloff = e_config.rx - e_config.screencols + 1;
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
            if (c == PAGE_UP) {
                e_config.cy = e_config.rowoff;
            } else if (c == PAGE_DOWN) {
                e_config.cy = e_config.rowoff + e_config.screenrows - 1;
                if (e_config.cy > e_config.numrows) {
                    e_config.cy = e_config.numrows;
                }
            }

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
            if (e_config.cy < e_config.numrows) {
                e_config.cx = e_config.row[e_config.cy].size;
            } 
            break;

        case ARROW_UP:
        case ARROW_LEFT:
        case ARROW_DOWN:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
    }
}

void editorDrawRows(struct abuff *ab) {
    for (int i = 0; i < e_config.screenrows; i++) {
        int filerow = i + e_config.rowoff;
        if (filerow >= e_config.numrows) {
            if (e_config.numrows == 0 && i == e_config.screenrows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome), "HYDRA EDITOR -- VERSION %s", HYDRA_VERSION);
                
                if (welcomelen > e_config.screencols) {
                    welcomelen = e_config.screencols;
                }

                int padding = (e_config.screencols - welcomelen) / 2;
                if (padding) {
                    abuffAppend(ab, "~", 1);
                    padding--;
                }

                while (padding--) {
                    abuffAppend(ab, " ", 1);
                }

                abuffAppend(ab, welcome, welcomelen);
            } else {
                abuffAppend(ab, "~", 1);
            }
        } else {
            int len = e_config.row[filerow].rsize - e_config.coloff;
            
            if (len < 0) {
                len = 0;
            }

            if (len > e_config.screencols) {
                len = e_config.screencols;
            }

            abuffAppend(ab, &e_config.row[filerow].render[e_config.coloff], len);
        }

        abuffAppend(ab, "\x1b[K", 3);
        abuffAppend(ab, "\r\n", 2);
    }
}

void editorDrawStatusBar(struct abuff* ab) {
    char status[80], rstatus[80];
    
    abuffAppend(ab, "\x1b[7m", 4); // deixa as cores invertidas

    int len = snprintf(status, sizeof(status), "%.20s - %d lines",
        e_config.filename ? e_config.filename : "[No Name]", e_config.numrows);

    int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
        e_config.cy + 1, e_config.numrows);

    if (len > e_config.screencols) {
        len = e_config.screencols;
    }

    abuffAppend(ab, status, len);

    while (len < e_config.screencols) {
        if (e_config.screencols - len == rlen) {
            abuffAppend(ab, rstatus, rlen);
            break;
        } else {
            abuffAppend(ab, " ", 1);
            len++;
        }
    }

    abuffAppend(ab, "\x1b[m", 3); // volta para as cores padrao
} 

void editorRefreshScreen() {
    editorScroll();

    struct abuff ab = ABUFF_INIT;

    abuffAppend(&ab, "\x1b[?25l", 6);
    abuffAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);
    editorDrawStatusBar(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", 
        (e_config.cy - e_config.rowoff) + 1, (e_config.rx - e_config.coloff) + 1);
    abuffAppend(&ab, buf, strlen(buf));

    abuffAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abuffFree(&ab);
}


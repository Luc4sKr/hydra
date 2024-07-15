#include <sys/ioctl.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "config.h"
#include "fileio.h"
#include "editor.h"
#include "terminal.h"

/*** append buffer ****/

void abuffAppend(struct abuff *ab, const char *s, int len) {
    char *new = realloc(ab->b, ab->len + len);

    if (new == NULL)
        return;

    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void abuffFree(struct abuff *ab) {
    free(ab->b);
}

/*** output ***/

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
            int len = e_config.row[filerow].size;
            if (len > e_config.screencols) {
                len = e_config.screencols;
            }

            abuffAppend(ab, e_config.row[filerow].chars, len);
        }

        abuffAppend(ab, "\x1b[K", 3);
        if (i < e_config.screenrows - 1) {
            abuffAppend(ab, "\r\n", 2);
        }
    }
}

void editorRefreshScreen() {
    editorScroll();

    struct abuff ab = ABUFF_INIT;

    abuffAppend(&ab, "\x1b[?25l", 6);
    abuffAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (e_config.cy - e_config.rowoff) + 1, e_config.cx + 1);
    abuffAppend(&ab, buf, strlen(buf));

    abuffAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abuffFree(&ab);
}

/*** input ***/

void editorMoveCursor(int key) {
    switch (key) {
        case ARROW_UP:
            if (e_config.cy != 0) {
                e_config.cy--;
            }
            break;
        case ARROW_LEFT:
            if (e_config.cx != 0) {
                e_config.cx--;
            }
            break;
        case ARROW_DOWN:
            if (e_config.cy < e_config.numrows) {
                e_config.cy++;
            }
            break;
        case ARROW_RIGHT:
            if (e_config.cx != e_config.screencols - 1) {
                e_config.cx++;
            }
            break;
    }
}

void editorProcessKeypress() {
    int c = editorReadKey();

    switch (c)
    {
    case CTRL_KEY('q'):
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
        break;

    case PAGE_UP:
    case PAGE_DOWN:
    {
        int times = e_config.screenrows;
        while (times--)
        {
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

int main(int argc, char *argv[]) {
    enableRawMode();
    initEditor();

    if (argc >= 2)
    {
        editorOpen(argv[1]);
    }

    while (1)
    {
        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}
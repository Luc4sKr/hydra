#include <sys/ioctl.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "config.h"
#include "abuff.h"
#include "fileio.h"
#include "editor.h"
#include "terminal.h"

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
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", 
        (e_config.cy - e_config.rowoff) + 1, (e_config.cx - e_config.coloff) + 1);
    abuffAppend(&ab, buf, strlen(buf));

    abuffAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abuffFree(&ab);
}

int main(int argc, char *argv[]) {
    enableRawMode();
    initEditor();

    if (argc >= 2) {
        editorOpenFile(argv[1]);
    }

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}
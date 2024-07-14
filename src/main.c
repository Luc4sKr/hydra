#include <sys/ioctl.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <string.h>

#include "config.h"
#include "fileio.h"
#include "editor.h"

/*** terminal ***/

void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &e_config.orig_termios))
        die("tcsetattr");
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &e_config.orig_termios))
        die("tcgetattr");
    
    atexit(disableRawMode);

    struct termios raw = e_config.orig_termios;

    // desativa algumas flags
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); 
    raw.c_cflag |= (CS8);

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) 
        die("tcsetattr");
}

int editorReadKey() {
    int nread;
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN)
            die("read");
    }

    if (c == '\x1b') {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) {
            return '\x1b';
        }

        if (read(STDIN_FILENO, &seq[1], 1) != 1) {
            return '\x1b';
        }

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDERR_FILENO, &seq[2], 1) != 1) {
                    return '\x1b';
                }

                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }

        return '\x1b';
    } else {
        return c;
    }
}

int getCursorPosition(int *rows, int *cols) {
    char buffer[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
        return -1;

    while (i < sizeof(buffer) -1) {
        if (read(STDIN_FILENO, &buffer[i], 1) != 1) break;
        if (buffer[i] == 'R') break;
        i++;
    }

    buffer[i] = '\0';

    if (buffer[0] != '\x1b' || buffer[1] != '[') return -1;
    if (sscanf(&buffer[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        // move o cursor para o final da tela
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
            return -1;
        }

        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/*** append buffer ****/

void abuffAppend(struct abuff *ab, const char *s, int len) {
    char *new = realloc(ab->b, ab->len + len);

    if (new == NULL) return;

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
        if (i >= e_config.numrows) {
            if (i == e_config.screenrows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                    "HYDRA TEXT -- VERSION %s", HYDRA_VERSION);
                
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
            int len = e_config.row.size;
            if (len > e_config.screencols) {
                len = e_config.screencols;
            }

            abuffAppend(ab, e_config.row.chars, len);
        }

        abuffAppend(ab, "\x1b[K", 3);
        if (i < e_config.screenrows - 1) {
            abuffAppend(ab, "\r\n", 2);
        }
    }
}

void editorRefreshScreen() {
    struct abuff ab = ABUFF_INIT;

    abuffAppend(&ab, "\x1b[?25l", 6);
    abuffAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", e_config.cy + 1, e_config.cx + 1);
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
            if (e_config.cy != e_config.screenrows - 1) {
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

/*** init ***/

void initEditor() {
    e_config.cx = 0;
    e_config.cy = 0;
    e_config.numrows = 0;

    if (getWindowSize(&e_config.screenrows, &e_config.screencols) == -1)
        die("getWindowSize");
}

int main() {
    enableRawMode();
    initEditor();
    editorOpen();

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}
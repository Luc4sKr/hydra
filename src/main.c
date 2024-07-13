#include <sys/ioctl.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <string.h>

#include "config.h"
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

char editorReadKey() {
    int nread;
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN)
            die("read");
    }

    return c;
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
    for (int i = 0; i < e_config.screen_rows; i++) {
        if (i == e_config.screen_rows / 3) {
            char welcome[80];
            int welcomelen = snprintf(welcome, sizeof(welcome),
                "HYDRA TEXT -- VERSION %s", HYDRA_VERSION);
            
            if (welcomelen > e_config.screen_cols) {
                welcomelen = e_config.screen_cols;
            }

            int padding = (e_config.screen_cols - welcomelen) / 2;
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

        abuffAppend(ab, "\x1b[K", 3);
        if (i < e_config.screen_rows - 1) {
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

void editorMoveCursor(char key) {
    switch (key) {
        case 'w':
            e_config.cy--;
            break;
        case 'a':
            e_config.cx--;
            break;
        case 's':
            e_config.cy++;
            break;
        case 'd':
            e_config.cx++;
            break;
    }
}

void editorProcessKeypress() {
    char c = editorReadKey();

    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
        case 'w':
        case 'a':
        case 's':
        case 'd':
            editorMoveCursor(c);
            break;
    }
}

/*** init ***/

void initEditor() {
    e_config.cx = 0;
    e_config.cy = 0;

    if (getWindowSize(&e_config.screen_rows, &e_config.screen_cols) == -1)
        die("getWindowSize");
}

int main() {
    enableRawMode();
    initEditor();

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}
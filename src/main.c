#include "config.h"
#include "fileio.h"
#include "editor.h"
#include "terminal.h"

int main(int argc, char *argv[]) {
    enableRawMode();
    initEditor();

    if (argc >= 2) {
        editorOpenFile(argv[1]);
    }

    editorSetStatusMessage("HELP: Ctrl-Q = quit");

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}
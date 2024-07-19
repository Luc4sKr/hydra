#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "fileio.h"
#include "editor.h"
#include "terminal.h"

void editorOpenFile(char* filename) {
    FILE* fp = fopen(filename, "r");

    if (!fp) {
        die("fopen");
    }

    char* line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
            linelen--;
        }

        editorAppendRows(line, linelen);
    }

    free(line);
    fclose(fp);
}
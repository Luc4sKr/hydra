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

void editorOpen(char* filename) {
    FILE* fp = fopen(filename, "r");

    if (!fp) {
        die("fopen");
    }

    char* line = NULL;
    size_t linecap = 0;
    ssize_t linelen = getline(&line, &linecap, fp);

    if (linelen != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
            linelen--;
        }

        e_config.row.size = linelen;
        e_config.row.chars = malloc(linelen + 1);
        memcpy(e_config.row.chars, line, linelen);
        e_config.row.chars[linelen] = '\0';
        e_config.numrows = 1;
    }

    free(line);
    fclose(fp);
}
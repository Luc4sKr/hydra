#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "fileio.h"
#include "editor.h"

void editorOpen() {
    char *line = "hello world";
    ssize_t linelen = 12;

    e_config.row.size = linelen;
    e_config.row.chars = malloc(linelen + 1);
    memcpy(e_config.row.chars, line, linelen);
    e_config.row.chars[linelen] = '\0';
    e_config.numrows = 1;
}
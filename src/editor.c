#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "editor.h"

struct editorConfig e_config;

void editorAppendRows(char* s, size_t len) {
    e_config.row = realloc(e_config.row, sizeof(editor_row) * (e_config.numrows + 1));

    int at = e_config.numrows;
    e_config.row[at].size = len;
    e_config.row[at].chars = malloc(len + 1);
    memcpy(e_config.row[at].chars, s, len);
    e_config.row[at].chars[len] = '\0';
    e_config.numrows++;
}
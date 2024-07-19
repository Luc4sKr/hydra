#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "abuff.h"

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

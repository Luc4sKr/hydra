#ifndef ABUFF_H
#define ABUFF_H

/*** append buffer ***/
struct abuff {
    char* b;
    int len;
};

void abuffAppend(struct abuff *ab, const char *s, int len);
void abuffFree(struct abuff *ab);

#endif
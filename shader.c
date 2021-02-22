//
// Created by dbrent on 2/20/21.
//

#include "shader.h"

#include <stdio.h>
#include <stdlib.h>

char *file_to_buffer(char *file) {
    FILE *f;
    long size;
    char *buffer;

    f = fopen(file, "r");
    if (f == NULL) {
        fprintf(stderr, "Failed to open file %s\n", file);
        return NULL;
    }

    fseek(f, 0L, SEEK_END);
    size = ftell(f);
    fseek(f, 0L, SEEK_SET);

    buffer = calloc(size, sizeof(char));
    if (buffer == NULL) {
        fprintf(stderr, "Could not create file buffer.\n");
        return NULL;
    }

    fread(buffer, sizeof(char), size, f);
    fclose(f);

    fprintf(stderr, "Successfully read file %s\n", file);
    return buffer;
}
#include "jellyfish.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int levenshtein_distance(const JFISH_UNICODE *s1, int s1_len, const JFISH_UNICODE *s2, int s2_len)
{
    size_t rows = s1_len + 1;
    size_t cols = s2_len + 1;
    size_t i, j;

    unsigned result;
    unsigned d1, d2, d3;
    unsigned *dist = safe_matrix_malloc(rows, cols, sizeof(unsigned));
    if (!dist) {
        return -1;
    }

    for (i = 0; i < rows; i++) {
        dist[i * cols] = i;
    }


    for (j = 0; j < cols; j++) {
        dist[j] = j;
    }

    for (j = 1; j < cols; j++) {
        for (i = 1; i < rows; i++) {
            if (s1[i - 1] == s2[j - 1]) {
                dist[(i * cols) + j] = dist[((i - 1) * cols) + (j - 1)];
            } else {
                d1 = dist[((i - 1) * cols) + j] + 1;
                d2 = dist[(i * cols) + (j - 1)] + 1;
                d3 = dist[((i - 1) * cols) + (j - 1)] + 1;

                dist[(i * cols) + j] = MIN(d1, MIN(d2, d3));
            }
        }
    }

    result = dist[(cols * rows) - 1];

    free(dist);

    return result;
}

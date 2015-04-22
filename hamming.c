#include "jellyfish.h"
#include <ctype.h>

size_t hamming_distance(const Py_UNICODE *s1, int len1,
                        const Py_UNICODE *s2, int len2) {
    unsigned distance = 0;
    int i1 = 0;
    int i2 = 0;

    for (; i1 < len1 && i2 < len2; i1++, i2++, s1++, s2++) {
        if (*s1 != *s2) {
            distance++;
        }
    }

    for ( ; i1 < len1; i1++, s1++) {
        distance++;
    }

    for ( ; i2 < len2; i2++, s2++) {
        distance++;
    }

    return distance;
}

#include "jellyfish.h"
#include <string.h>
#include <ctype.h>

#define ISVOWEL(c) ((c) == 'A' || (c) == 'E' || (c) == 'I' || \
                    (c) == 'O' || (c) == 'U')

#define TRUE 1
#define FALSE 0

static size_t compute_match_rating_codex(const JFISH_UNICODE *str, size_t len, JFISH_UNICODE codex[7]);

int match_rating_comparison(const JFISH_UNICODE *s1, size_t len1, const JFISH_UNICODE *s2, size_t len2) {
    /* s1 and s2 are already in uppercase when this function is called */
    size_t s1c_len, s2c_len;
    size_t i, j;
    int diff;
    JFISH_UNICODE *longer;

    JFISH_UNICODE s1_codex[7], s2_codex[7];
    s1c_len = compute_match_rating_codex(s1, len1, s1_codex);
    s2c_len = compute_match_rating_codex(s2, len2, s2_codex);

    if (abs(s1c_len - s2c_len) >= 3) {
        return -1;
    }

    for (i = 0; i < s1c_len && i < s2c_len; i++) {
        if (s1_codex[i] == s2_codex[i]) {
            s1_codex[i] = ' ';
            s2_codex[i] = ' ';
        }
    }

    i = s1c_len - 1;
    j = s2c_len - 1;

    if (s1c_len == 0 && s2c_len == 0) {
        return -1;
    }

    while (i != 0 && j != 0) {
        if (s1_codex[i] == ' ') {
            i--;
            continue;
        }

        if (s2_codex[j] == ' ') {
            j--;
            continue;
        }

        if (s1_codex[i] == s2_codex[j]) {
            s1_codex[i] = ' ';
            s2_codex[j] = ' ';
        }

        i--;
        j--;
    }

    if (s1c_len > s2c_len) {
        longer = s1_codex;
    } else {
        longer = s2_codex;
    }

    for (diff = 0; *longer; longer++) {
        if (*longer != ' ') {
            diff++;
        }
    }

    diff = 6 - diff;
    i = s1c_len + s2c_len;

    if (i <= 4) {
        return diff >= 5;
    } else if (i <= 7) {
        return diff >= 4;
    } else if (i <= 11) {
        return diff >= 3;
    } else {
        return diff >= 2;
    }
}

JFISH_UNICODE* match_rating_codex(const JFISH_UNICODE *str, size_t len) {
    JFISH_UNICODE *codex = malloc(7 * sizeof(JFISH_UNICODE));
    if (!codex) {
        return NULL;
    }
    compute_match_rating_codex(str, len, codex);

    return codex;
}

static size_t compute_match_rating_codex(const JFISH_UNICODE *str, size_t len, JFISH_UNICODE codex[7]) {
    /* str is already in uppercase when this function is called */
    size_t i, j;
    int first;
    JFISH_UNICODE c, prev;

    prev = '\0';
    first = TRUE;
    for(i = 0, j = 0; i < len && j < 7; i++) {
        c = str[i];
        if (!ISALPHA(c)) {
            prev = c;
            continue;
        }

        if (first || (!ISVOWEL(c) && c != prev)) {
            if (j == 6) {
                codex[3] = codex[4];
                codex[4] = codex[5];
                j = 5;
            }

            codex[j++] = c;
        }
        prev = c;
        first = FALSE;
    }

    codex[j] = '\0';
    return j;
}

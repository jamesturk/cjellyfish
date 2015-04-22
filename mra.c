#include "jellyfish.h"
#include <string.h>
#include <ctype.h>

static size_t compute_match_rating_codex(const JFISH_UNICODE *str, size_t len, JFISH_UNICODE codex[7]);

int match_rating_comparison(const JFISH_UNICODE *s1, size_t len1, const JFISH_UNICODE *s2, size_t len2) {
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
    size_t i, j;
    JFISH_UNICODE c, prev;

    prev = '\0';
    for(i = 0, j = 0; i < len && j < 7; i++) {
        c = toupper(str[i]);

        if (c == ' ' || (i != 0 && (c == 'A' || c == 'E' || c == 'I' ||
                                    c == 'O' || c == 'U'))) {
            continue;
        }

        if (c == prev) {
            continue;
        }

        if (j == 6) {
            codex[3] = codex[4];
            codex[4] = codex[5];
            j = 5;
        }

        codex[j++] = c;
    }

    codex[j] = '\0';
    return j;
}

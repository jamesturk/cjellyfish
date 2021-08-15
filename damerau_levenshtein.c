#include "jellyfish.h"
#include <string.h>
#include <stdio.h>
#include <wchar.h>

/*

  Trie is a nested search tree, where each node's key is broken down into parts
  and looking up a certain key means a sequence of lookups in small associative
  arrays. They are usually used for strings, where a word "foo" would be
  basically looked up as trie["f"]["o"]["o"].

  In this case, the tries split the incoming integer into segments, omitting
  upper zero ones, and they are looked up as follows:

  I.e. for segments of 1 byte,

  - for key = 0x11, the result is d->values[0x11]
  - for key = 0x1122  -  d->child_nodes[0x11]->values[0x22]
  - for key = 0x112233  -  d->child_nodes[0x11]->child_nodes[0x22]->values[0x33]

  And so on.

  Child nodes are created on demand, when a value should be stored in them.

  If no value is stored in the trie for a certain key, the lookup returns 0.

*/


#define TRIE_VALUES_PER_LEVEL 256
/* Each level takes one byte from dictionary key, hence max levels is: */
#define TRIE_MAX_LEVELS sizeof(size_t)

struct trie {
    size_t* values;
    struct trie** child_nodes;
};


struct trie* trie_create(void)
{
    return calloc(1, sizeof(struct trie));
}


void trie_destroy(struct trie* d)
{
    size_t i;
    if (!d) {
        return;
    }
    free(d->values);
    if (d->child_nodes) {
        for (i = 0; i < TRIE_VALUES_PER_LEVEL; ++i) {
            trie_destroy(d->child_nodes[i]);
        }
    }
    free(d->child_nodes);
    free(d);
}


size_t trie_get(struct trie* d, size_t key)
{
    size_t level_keys[TRIE_MAX_LEVELS];
    size_t level_pos = 0;

    size_t cur_remainder = key;
    size_t cur_key;
    while (1) {
        level_keys[level_pos] = cur_remainder % TRIE_VALUES_PER_LEVEL;
        cur_remainder /= TRIE_VALUES_PER_LEVEL;
        if (!cur_remainder) {
            break;
        }
        ++level_pos;
    }

    while (level_pos) {
        cur_key = level_keys[level_pos];
        if (!d->child_nodes || !d->child_nodes[cur_key]) {
            return 0;
        }
        d = d->child_nodes[cur_key];
        --level_pos;
    }
    if (!d->values) {
        return 0;
    }
    return d->values[level_keys[0]];
}


int trie_set(struct trie* d, size_t key, size_t val)
{
    size_t level_keys[TRIE_MAX_LEVELS];
    size_t level_pos = 0;

    size_t cur_remainder = key;
    size_t cur_key;
    while (1) {
        level_keys[level_pos] = cur_remainder % TRIE_VALUES_PER_LEVEL;
        cur_remainder /= TRIE_VALUES_PER_LEVEL;
        if (!cur_remainder) {
            break;
        }
        ++level_pos;
    }

    while (level_pos) {
        cur_key = level_keys[level_pos];
        if (!d->child_nodes) {
            d->child_nodes = calloc(TRIE_VALUES_PER_LEVEL, sizeof(struct trie*));
            if (!d->child_nodes) {
                return 0;
            }
        }
        if (!d->child_nodes[cur_key]) {
            d->child_nodes[cur_key] = trie_create();
            if (!d->child_nodes[cur_key]){
                return 0;
            }
        }
        d = d->child_nodes[cur_key];
        --level_pos;
    }

    if (!d->values) {
        d->values = calloc(TRIE_VALUES_PER_LEVEL, sizeof(size_t));
        if (!d->values) {
            return 0;
        }
    }
    d->values[level_keys[0]] = val;
    return 1;
}


int damerau_levenshtein_distance(const JFISH_UNICODE *s1, const JFISH_UNICODE *s2, size_t len1, size_t len2)
{
    size_t infinite = len1 + len2;
    size_t cols = len2 + 2;

    size_t i, j, i1, j1;
    size_t db;
    size_t d1, d2, d3, d4, result;
    unsigned short cost;

    size_t *dist = NULL;

    struct trie* da = trie_create();
    if (!da) {
        return -1;
    }

    dist = safe_matrix_malloc((len1 + 2), cols, sizeof(size_t));
    if (!dist) {
        result = -1;
        goto cleanup_da;
    }

    dist[0] = infinite;

    for (i = 0; i <= len1; i++) {
        dist[((i + 1) * cols) + 0] = infinite;
        dist[((i + 1) * cols) + 1] = i;
    }

    for (i = 0; i <= len2; i++) {
        dist[i + 1] = infinite;       // 0*cols + row
        dist[cols + i + 1] = i;       // 1*cols + row
    }

    for (i = 1; i <= len1; i++) {
        db = 0;
        for (j = 1; j <= len2; j++) {
            i1 = trie_get(da, s2[j-1]);
            j1 = db;

            if (s1[i - 1] == s2[j - 1]) {
                cost = 0;
                db = j;
            } else {
                cost = 1;
            }

            d1 = dist[(i * cols) + j] + cost;
            d2 = dist[((i + 1) * cols) + j] + 1;
            d3 = dist[(i * cols) + j + 1] + 1;
            d4 = dist[(i1 * cols) + j1] + (i - i1 - 1) + 1 + (j - j1 - 1);

            dist[((i+1)*cols) + j + 1] = MIN(MIN(d1, d2), MIN(d3, d4));
        }

        if (!trie_set(da, s1[i-1], i)) {
            result = -1;
            goto cleanup;
        };
    }

    result = dist[((len1+1) * cols) + len2 + 1];


 cleanup:
    free(dist);

 cleanup_da:
    trie_destroy(da);

    return result;
}

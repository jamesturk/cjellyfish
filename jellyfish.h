#ifndef _JELLYFISH_H_
#define _JELLYFISH_H_

#include <stdlib.h>

#if CJELLYFISH_PYTHON
#include <Python.h>
#define JFISH_UNICODE Py_UCS4
#define ISALPHA Py_UNICODE_ISALPHA
#else
#include <wctype.h>
#include <wchar.h>
#define JFISH_UNICODE wint_t
#define ISALPHA iswalpha
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

static inline void* safe_malloc(size_t num, size_t size)
{
    size_t alloc_size = num * size;
    if (alloc_size / num != size)
    {
        return NULL;
    }
    return malloc(alloc_size);
}

static inline void* safe_matrix_malloc(size_t rows, size_t cols, size_t size)
{
    size_t matrix_size = rows * cols;
    if (matrix_size / rows != cols)
    {
        return NULL;
    }
    return safe_malloc(matrix_size, size);
}

double jaro_winkler_similarity(const JFISH_UNICODE *str1, int len1, const JFISH_UNICODE *str2, int len2, int long_tolerance);
double jaro_similarity(const JFISH_UNICODE *str1, int len1, const JFISH_UNICODE *str2, int len2);

size_t hamming_distance(const JFISH_UNICODE *str1, int len1,
        const JFISH_UNICODE *str2, int len2);

int levenshtein_distance(const JFISH_UNICODE *str1, int len1, const JFISH_UNICODE *str2, int len2);

int damerau_levenshtein_distance(const JFISH_UNICODE *str1, const JFISH_UNICODE *str2,
        size_t len1, size_t len2);

char* soundex(const char *str);

char* metaphone(const char *str);

JFISH_UNICODE *nysiis(const JFISH_UNICODE *str, int len);

JFISH_UNICODE* match_rating_codex(const JFISH_UNICODE *str, size_t len);
int match_rating_comparison(const JFISH_UNICODE *str1, size_t len1, const JFISH_UNICODE *str2, size_t len2);

struct stemmer;
extern struct stemmer * create_stemmer(void);
extern void free_stemmer(struct stemmer * z);
extern int stem(struct stemmer * z, JFISH_UNICODE * b, int k);

#endif

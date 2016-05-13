#include <Python.h>
#include <math.h>
#include "jellyfish.h"

struct jellyfish_state {
    PyObject *unicodedata_normalize;
};

#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct jellyfish_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
static struct jellyfish_state _state;
#endif

#ifdef _MSC_VER
#define INLINE __inline
#else
#define INLINE inline
#endif

#if PY_MAJOR_VERSION >= 3
#define UTF8_BYTES(s) (PyBytes_AS_STRING(s))
#define NO_BYTES_ERR_STR "expected str, got bytes"
#else
#define UTF8_BYTES(s) (PyString_AS_STRING(s))
#define NO_BYTES_ERR_STR "expected unicode, got str"
#endif

#define UNSUPPORTED_CODEPOINT "Encountered unsupported code point in string."


/* Returns a new reference to a PyString (python < 3) or
 * PyBytes (python >= 3.0).
 *
 * If passed a PyUnicode, the returned object will be NFKD UTF-8.
 * If passed a PyString or PyBytes no conversion is done.
 */
static INLINE PyObject* normalize(PyObject *mod, PyObject *pystr) {
    PyObject *unicodedata_normalize;
    PyObject *normalized;
    PyObject *utf8;

    if (PyUnicode_Check(pystr)) {
        unicodedata_normalize = GETSTATE(mod)->unicodedata_normalize;
        normalized = PyObject_CallFunction(unicodedata_normalize,
                                           "sO", "NFKD", pystr);
        if (!normalized) {
            return NULL;
        }
        utf8 = PyUnicode_AsUTF8String(normalized);
        Py_DECREF(normalized);
        return utf8;
    }

    PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
    return NULL;
}

static PyObject * jellyfish_jaro_winkler(PyObject *self, PyObject *args, PyObject *kw)
{
    const Py_UNICODE *s1, *s2;
    int len1, len2;
    double result;
    int long_tolerance = 0;
    static char *keywords[] = {"s1", "s2", "long_tolerance", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kw, "u#u#|i", keywords, &s1, &len1, &s2, &len2, &long_tolerance)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }

    result = jaro_winkler(s1, len1, s2, len2, long_tolerance);
    // jaro returns a big negative number on error, don't use
    // 0 here in case there's floating point inaccuracy
    // .. used to use NaN but different compilers (*cough*MSVC*cough)
    // handle it really poorly
    if (result < -1) {
        PyErr_NoMemory();
        return NULL;
    }

    return Py_BuildValue("d", result);
}

static PyObject * jellyfish_jaro_distance(PyObject *self, PyObject *args)
{
    const Py_UNICODE *s1, *s2;
    int len1, len2;
    double result;

    if (!PyArg_ParseTuple(args, "u#u#", &s1, &len1, &s2, &len2)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }

    result = jaro_distance(s1, len1, s2, len2);
    // see earlier note about jaro_distance return value
    if (result < -1) {
        PyErr_NoMemory();
        return NULL;
    }

    return Py_BuildValue("d", result);
}

static PyObject * jellyfish_hamming_distance(PyObject *self, PyObject *args)
{
    const Py_UNICODE *s1, *s2;
    int len1, len2;
    unsigned result;

    if (!PyArg_ParseTuple(args, "u#u#", &s1, &len1, &s2, &len2)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }

    result = hamming_distance(s1, len1, s2, len2);

    return Py_BuildValue("I", result);
}

static PyObject* jellyfish_levenshtein_distance(PyObject *self, PyObject *args)
{
    const Py_UNICODE *s1, *s2;
    int len1, len2;
    int result;

    if (!PyArg_ParseTuple(args, "u#u#", &s1, &len1, &s2, &len2)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }

    result = levenshtein_distance(s1, len1, s2, len2);
    if (result == -1) {
        // levenshtein_distance only returns failure code (-1) on
        // failed malloc
        PyErr_NoMemory();
        return NULL;
    }

    return Py_BuildValue("i", result);
}


static PyObject* jellyfish_damerau_levenshtein_distance(PyObject *self,
                                                        PyObject *args)
{
    Py_UNICODE *s1, *s2;
    int len1, len2;
    int result;

    if (!PyArg_ParseTuple(args, "u#u#", &s1, &len1, &s2, &len2)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }

    result = damerau_levenshtein_distance(s1, s2, len1, len2);
    if (result == -1) {
        PyErr_NoMemory();
        return NULL;
    }
    else if (result == -2) {
        PyErr_SetString(PyExc_ValueError, UNSUPPORTED_CODEPOINT);
        return NULL;
    }

    return Py_BuildValue("i", result);
}

static PyObject* jellyfish_soundex(PyObject *self, PyObject *args)
{
    PyObject *pystr;
    PyObject *normalized;
    PyObject* ret;
    char *result;

    if (!PyArg_ParseTuple(args, "O", &pystr)) {
        return NULL;
    }

    normalized = normalize(self, pystr);
    if (!normalized) {
        return NULL;
    }

    result = soundex(UTF8_BYTES(normalized));
    Py_DECREF(normalized);

    if (!result) {
        // soundex only fails on bad malloc
        PyErr_NoMemory();
        return NULL;
    }

    ret = Py_BuildValue("s", result);
    free(result);

    return ret;
}

static PyObject* jellyfish_metaphone(PyObject *self, PyObject *args)
{
    PyObject *pystr;
    PyObject *normalized;
    PyObject *ret;
    char *result;

    if (!PyArg_ParseTuple(args, "O", &pystr)) {
        return NULL;
    }

    normalized = normalize(self, pystr);
    if (!normalized) {
        return NULL;
    }

    result = metaphone((const char*)UTF8_BYTES(normalized));
    Py_DECREF(normalized);

    if (!result) {
        // metaphone only fails on bad malloc
        PyErr_NoMemory();
        return NULL;
    }

    ret = Py_BuildValue("s", result);
    free(result);

    return ret;
}

static PyObject* jellyfish_match_rating_codex(PyObject *self, PyObject *args)
{
    const Py_UNICODE *str;
    int len;
    Py_UNICODE *result;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "u#", &str, &len)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }

    result = match_rating_codex(str, len);
    if (!result) {
        PyErr_NoMemory();
        return NULL;
    }

    ret = Py_BuildValue("u", result);
    free(result);

    return ret;
}

static PyObject* jellyfish_match_rating_comparison(PyObject *self,
                                                   PyObject *args)
{
    const Py_UNICODE *str1, *str2;
    int len1, len2;
    int result;

    if (!PyArg_ParseTuple(args, "u#u#", &str1, &len1, &str2, &len2)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }

    result = match_rating_comparison(str1, len1, str2, len2);

    if (result == -1) {
        Py_RETURN_NONE;
    } else if (result) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

static PyObject* jellyfish_nysiis(PyObject *self, PyObject *args)
{
    const Py_UNICODE *str;
    Py_UNICODE *result;
    int len;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "u#", &str, &len)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }

    result = nysiis(str, len);
    if (!result) {
        PyErr_NoMemory();
        return NULL;
    }

    ret = Py_BuildValue("u", result);
    free(result);

    return ret;
}

static PyObject* jellyfish_porter_stem(PyObject *self, PyObject *args)
{
    const Py_UNICODE *str;
    int len;
    Py_UNICODE *result;
    PyObject *ret;
    struct stemmer *z;
    int end;

    if (!PyArg_ParseTuple(args, "u#", &str, &len)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }

    z = create_stemmer();
    if (!z) {
        PyErr_NoMemory();
        return NULL;
    }

    result = malloc((len+1) * sizeof(Py_UNICODE));
    if (!result) {
        free_stemmer(z);
        PyErr_NoMemory();
        return NULL;
    }
    memcpy(result, str, len * sizeof(Py_UNICODE));

    end = stem(z, result, len - 1);
    result[end + 1] = '\0';

    ret = Py_BuildValue("u", result);

    free(result);
    free_stemmer(z);

    return ret;
}

static PyMethodDef jellyfish_methods[] = {
    {"jaro_winkler", (PyCFunction)jellyfish_jaro_winkler, METH_VARARGS|METH_KEYWORDS,
     "jaro_winkler(string1, string2, long_tolerance)\n\n"
     "Do a Jaro-Winkler string comparison between string1 and string2."},

    {"jaro_distance", jellyfish_jaro_distance, METH_VARARGS,
     "jaro_distance(string1, string2)\n\n"
     "Get a Jaro string distance metric for string1 and string2."},

    {"hamming_distance", jellyfish_hamming_distance, METH_VARARGS,
     "hamming_distance(string1, string2)\n\n"
     "Compute the Hamming distance between string1 and string2."},

    {"levenshtein_distance", jellyfish_levenshtein_distance, METH_VARARGS,
     "levenshtein_distance(string1, string2)\n\n"
     "Compute the Levenshtein distance between string1 and string2."},

    {"damerau_levenshtein_distance", jellyfish_damerau_levenshtein_distance,
     METH_VARARGS,
     "damerau_levenshtein_distance(string1, string2)\n\n"
     "Compute the Damerau-Levenshtein distance between string1 and string2."},

    {"soundex", jellyfish_soundex, METH_VARARGS,
     "soundex(string)\n\n"
     "Calculate the soundex code for a given name."},

    {"metaphone", jellyfish_metaphone, METH_VARARGS,
     "metaphone(string)\n\n"
     "Calculate the metaphone representation of a given string."},

    {"match_rating_codex", jellyfish_match_rating_codex, METH_VARARGS,
     "match_rating_codex(string)\n\n"
     "Calculate the Match Rating Approach representation of a given string."},

    {"match_rating_comparison", jellyfish_match_rating_comparison, METH_VARARGS,
     "match_rating_comparison(string, string)\n\n"
     "Compute the Match Rating Approach similarity between string1 and"
     "string2."},

    {"nysiis", jellyfish_nysiis, METH_VARARGS,
     "nysiis(string)\n\n"
     "Compute the NYSIIS (New York State Identification and Intelligence\n"
     "System) code for a string."},

    {"porter_stem", jellyfish_porter_stem, METH_VARARGS,
     "porter_stem(string)\n\n"
     "Return the result of running the Porter stemming algorithm on "
     "a single-word string."},

    {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3
#define INITERROR return NULL

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "jellyfish.cjellyfish",
    NULL,
    sizeof(struct jellyfish_state),
    jellyfish_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyObject* PyInit_cjellyfish(void)
#else

#define INITERROR return

PyMODINIT_FUNC initcjellyfish(void)
#endif
{
    PyObject *unicodedata;

#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&moduledef);
#else
    PyObject *module = Py_InitModule("jellyfish.cjellyfish", jellyfish_methods);
#endif

    if (module == NULL) {
        INITERROR;
    }

    unicodedata = PyImport_ImportModule("unicodedata");
    if (!unicodedata) {
        INITERROR;
    }

    GETSTATE(module)->unicodedata_normalize =
        PyObject_GetAttrString(unicodedata, "normalize");
    Py_DECREF(unicodedata);

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}

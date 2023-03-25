#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <math.h>
#include "jellyfish.h"

struct jellyfish_state {
    PyObject *unicodedata_normalize;
};

#define GETSTATE(m) ((struct jellyfish_state*)PyModule_GetState(m))
#define UTF8_BYTES(s) (PyBytes_AS_STRING(s))
#define NO_BYTES_ERR_STR "str argument expected"

#ifdef _MSC_VER
#define INLINE __inline
#else
#define INLINE inline
#endif


/* Create PyUnicode object from NUL terminated Py_UCS4 string. */
static PyObject* unicode_from_ucs4(const Py_UCS4 *str)
{
    size_t len = 0;
    while (str[len]) len++;
    return PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, (void*)str, len);
}

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

static PyObject * jellyfish_jaro_winkler_similarity(PyObject *self, PyObject *args, PyObject *kw)
{
    PyObject *u1, *u2;
    Py_UCS4 *s1, *s2;
    Py_ssize_t len1, len2;
    double result;
    int long_tolerance = 0;
    static char *keywords[] = {"s1", "s2", "long_tolerance", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kw, "UU|i", keywords, &u1, &u2, &long_tolerance)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }
    s1 = PyUnicode_AsUCS4Copy(u1);
    if (s1 == NULL) {
        return NULL;
    }
    s2 = PyUnicode_AsUCS4Copy(u2);
    if (s2 == NULL) {
        PyMem_Free(s1);
        return NULL;
    }
    len1 = PyUnicode_GET_LENGTH(u1);
    len2 = PyUnicode_GET_LENGTH(u2);

    result = jaro_winkler_similarity(s1, len1, s2, len2, long_tolerance);
    PyMem_Free(s1);
    PyMem_Free(s2);

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

static PyObject * jellyfish_jaro_similarity(PyObject *self, PyObject *args)
{
    PyObject *u1, *u2;
    Py_UCS4 *s1, *s2;
    Py_ssize_t len1, len2;
    double result;

    if (!PyArg_ParseTuple(args, "UU", &u1, &u2)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }
    len1 = PyUnicode_GET_LENGTH(u1);
    len2 = PyUnicode_GET_LENGTH(u2);
    s1 = PyUnicode_AsUCS4Copy(u1);
    if (s1 == NULL) {
        return NULL;
    }
    s2 = PyUnicode_AsUCS4Copy(u2);
    if (s2 == NULL) {
        PyMem_Free(s1);
        return NULL;
    }

    result = jaro_similarity(s1, len1, s2, len2);
    PyMem_Free(s1);
    PyMem_Free(s2);

    // see earlier note about jaro_similarity return value
    if (result < -1) {
        PyErr_NoMemory();
        return NULL;
    }

    return Py_BuildValue("d", result);
}

static PyObject * jellyfish_hamming_distance(PyObject *self, PyObject *args)
{
    PyObject *u1, *u2;
    Py_UCS4 *s1, *s2;
    Py_ssize_t len1, len2;
    unsigned result;

    if (!PyArg_ParseTuple(args, "UU", &u1, &u2)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }
    len1 = PyUnicode_GET_LENGTH(u1);
    len2 = PyUnicode_GET_LENGTH(u2);
    s1 = PyUnicode_AsUCS4Copy(u1);
    if (s1 == NULL) {
        return NULL;
    }
    s2 = PyUnicode_AsUCS4Copy(u2);
    if (s2 == NULL) {
        PyMem_Free(s1);
        return NULL;
    }

    result = hamming_distance(s1, len1, s2, len2);
    PyMem_Free(s1);
    PyMem_Free(s2);

    return Py_BuildValue("I", result);
}

static PyObject* jellyfish_levenshtein_distance(PyObject *self, PyObject *args)
{
    PyObject *u1, *u2;
    Py_UCS4 *s1, *s2;
    Py_ssize_t len1, len2;
    int result;

    if (!PyArg_ParseTuple(args, "UU", &u1, &u2)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }
    len1 = PyUnicode_GET_LENGTH(u1);
    len2 = PyUnicode_GET_LENGTH(u2);
    s1 = PyUnicode_AsUCS4Copy(u1);
    if (s1 == NULL) {
        return NULL;
    }
    s2 = PyUnicode_AsUCS4Copy(u2);
    if (s2 == NULL) {
        PyMem_Free(s1);
        return NULL;
    }

    result = levenshtein_distance(s1, len1, s2, len2);
    PyMem_Free(s1);
    PyMem_Free(s2);

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
    PyObject *u1, *u2;
    Py_UCS4 *s1, *s2;
    Py_ssize_t len1, len2;
    int result;

    if (!PyArg_ParseTuple(args, "UU", &u1, &u2)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }
    len1 = PyUnicode_GET_LENGTH(u1);
    len2 = PyUnicode_GET_LENGTH(u2);
    s1 = PyUnicode_AsUCS4Copy(u1);
    if (s1 == NULL) {
        return NULL;
    }
    s2 = PyUnicode_AsUCS4Copy(u2);
    if (s2 == NULL) {
        PyMem_Free(s1);
        return NULL;
    }

    result = damerau_levenshtein_distance(s1, s2, len1, len2);
    PyMem_Free(s1);
    PyMem_Free(s2);
    if (result == -1) {
        PyErr_NoMemory();
        return NULL;
    }
    return Py_BuildValue("i", result);
}

static PyObject* jellyfish_soundex(PyObject *self, PyObject *args)
{
    PyObject *str;
    PyObject *normalized;
    PyObject* ret;
    char *result;

    if (!PyArg_ParseTuple(args, "U", &str)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }

    normalized = normalize(self, str);
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
    PyObject *str;
    PyObject *normalized;
    PyObject *ret;
    char *result;

    if (!PyArg_ParseTuple(args, "U", &str)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }

    normalized = normalize(self, str);
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
    PyObject *ustr, *ustr_upper;
    Py_UCS4 *str;
    Py_ssize_t len;
    Py_UCS4 *result;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "U", &ustr)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }
    ustr_upper = PyObject_CallMethod(ustr, "upper", NULL);
    len = PyUnicode_GET_LENGTH(ustr_upper);
    str = PyUnicode_AsUCS4Copy(ustr_upper);
    if (str == NULL) {
        Py_DECREF(ustr_upper);
        return NULL;
    }

    result = match_rating_codex(str, len);
    PyMem_Free(str);
    Py_DECREF(ustr_upper);
    if (!result) {
        PyErr_NoMemory();
        return NULL;
    }

    ret = unicode_from_ucs4(result);
    free(result);

    return ret;
}

static PyObject* jellyfish_match_rating_comparison(PyObject *self,
                                                   PyObject *args)
{
    PyObject *u1, *u2, *u_upper1, *u_upper2;
    Py_UCS4 *str1, *str2;
    Py_ssize_t len1, len2;
    int result;

    if (!PyArg_ParseTuple(args, "UU", &u1, &u2)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }
    u_upper1 = PyObject_CallMethod(u1, "upper", NULL);
    u_upper2 = PyObject_CallMethod(u2, "upper", NULL);
    len1 = PyUnicode_GET_LENGTH(u_upper1);
    len2 = PyUnicode_GET_LENGTH(u_upper2);
    str1 = PyUnicode_AsUCS4Copy(u_upper1);
    if (str1 == NULL) {
        Py_DECREF(u_upper1);
        Py_DECREF(u_upper2);
        return NULL;
    }
    str2 = PyUnicode_AsUCS4Copy(u_upper2);
    if (str2 == NULL) {
        PyMem_Free(str1);
        Py_DECREF(u_upper1);
        Py_DECREF(u_upper2);
        return NULL;
    }

    result = match_rating_comparison(str1, len1, str2, len2);
    PyMem_Free(str1);
    PyMem_Free(str2);
    Py_DECREF(u_upper1);
    Py_DECREF(u_upper2);

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
    PyObject *ustr;
    Py_UCS4 *str;
    Py_UCS4 *result;
    Py_ssize_t len;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "U", &ustr)) {
        PyErr_SetString(PyExc_TypeError, NO_BYTES_ERR_STR);
        return NULL;
    }
    str = PyUnicode_AsUCS4Copy(ustr);
    len = PyUnicode_GET_LENGTH(ustr);
    if (str == NULL) {
        return NULL;
    }

    result = nysiis(str, len);
    if (!result) {
        PyErr_NoMemory();
        return NULL;
    }

    ret = unicode_from_ucs4(result);
    free(result);

    return ret;
}


static PyMethodDef jellyfish_methods[] = {
    {"jaro_winkler_similarity", (PyCFunction)jellyfish_jaro_winkler_similarity, METH_VARARGS|METH_KEYWORDS,
     "jaro_winkler_similarity(string1, string2, long_tolerance)\n\n"
     "Do a Jaro-Winkler string comparison between string1 and string2."},

    {"jaro_similarity", jellyfish_jaro_similarity, METH_VARARGS,
     "jaro_similarity(string1, string2)\n\n"
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

    {NULL, NULL, 0, NULL}
};

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
{
    PyObject *unicodedata;
    PyObject *module = PyModule_Create(&moduledef);

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

    return module;
}

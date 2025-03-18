#define PY_SSIZE_T_CLEAN
#include "Python.h"

#include <util/digest/murmur.h>

#if PY_MAJOR_VERSION >= 3
  #define MOD_ERROR_VAL NULL
  #define MOD_SUCCESS_VAL(val) val
  #define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
  #define MOD_DEF(ob, name, doc, methods) \
          static struct PyModuleDef moduledef = { \
            PyModuleDef_HEAD_INIT, name, doc, -1, methods, NULL, NULL, NULL, NULL}; \
          ob = PyModule_Create(&moduledef);
#endif


static PyObject* hash64(PyObject*, PyObject* args, PyObject* kwargs) {
    static const char *kwlist[] = {"key", "seed", NULL};

    const char* text;
    Py_ssize_t length = 0;
    ui64 seed = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|K", const_cast<char**>(kwlist),
                                     &text, &length, &seed))
        return NULL;

    ui64 hash = MurmurHash<ui64>(text, length, seed);

    return Py_BuildValue("K", hash);
}

static PyMethodDef MurmurhashMethods[] = {
    {"hash64", (PyCFunction)hash64, METH_VARARGS|METH_KEYWORDS, "Caclulate MurmurHash64."},
    {NULL, NULL, 0, NULL} /* Sentinel */
};


#if PY_MAJOR_VERSION >= 3
MOD_INIT(_murmurhash)
{
  PyObject *m;

  MOD_DEF (m,
           "_murmurhash",
           "Murmuhash implementation.",
           MurmurhashMethods
          );
  if (!m)
    return MOD_ERROR_VAL;

  return MOD_SUCCESS_VAL(m);
}
#else
    PyMODINIT_FUNC init_murmurhash(void) {
        (void)Py_InitModule("_murmurhash", MurmurhashMethods);
    }
#endif

diff --git a/Objects/codeobject.c b/Objects/codeobject.c
index 353f414..506f5f4 100644
--- a/Objects/codeobject.c
+++ b/Objects/codeobject.c
@@ -152,6 +152,9 @@ PyCode_New(int argcount, int kwonlyargcount,
     co->co_lnotab = lnotab;
     co->co_zombieframe = NULL;
     co->co_weakreflist = NULL;
+    co->co_jitted = NULL;
+    co->co_runcount = 0;
+    co->co_extra = NULL;
     return co;
 }
 
@@ -370,6 +373,8 @@ code_dealloc(PyCodeObject *co)
     Py_XDECREF(co->co_filename);
     Py_XDECREF(co->co_name);
     Py_XDECREF(co->co_lnotab);
+    Py_XDECREF(co->co_jitted);
+    Py_XDECREF(co->co_extra);
     if (co->co_cell2arg != NULL)
         PyMem_FREE(co->co_cell2arg);
     if (co->co_zombieframe != NULL)
@@ -542,6 +547,69 @@ PyTypeObject PyCode_Type = {
     code_new,                           /* tp_new */
 };
 
+static PyObject *
+jittedcode_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
+{
+    if (PyTuple_GET_SIZE(args) || (kwargs && PyDict_Size(kwargs))) {
+        PyErr_SetString(PyExc_TypeError, "JittedCode takes no arguments");
+        return NULL;
+    }
+    return PyJittedCode_New(0);
+}
+
+PyObject* PyJittedCode_New() {
+    return PyObject_New(PyJittedCode, &PyJittedCode_Type);
+}
+
+static void
+jittedcode_dealloc(PyJittedCode *co) {
+    PyThreadState *tstate = PyThreadState_GET();
+    if (tstate->interp->jitfree != NULL) {
+        tstate->interp->jitfree(co);
+    }
+}
+
+PyTypeObject PyJittedCode_Type = {
+    PyVarObject_HEAD_INIT(&PyType_Type, 0)
+    "jittedcode",                       /* tp_name */
+    sizeof(PyJittedCode),               /* tp_basicsize */
+    0,                                  /* tp_itemsize */
+    jittedcode_dealloc,                 /* tp_dealloc */
+    0,                                  /* tp_print */
+    0,                                  /* tp_getattr */
+    0,                                  /* tp_setattr */
+    0,                                  /* tp_reserved */
+    0,                                  /* tp_repr */
+    0,                                  /* tp_as_number */
+    0,                                  /* tp_as_sequence */
+    0,                                  /* tp_as_mapping */
+    0,                                  /* tp_hash */
+    0,                                  /* tp_call */
+    0,                                  /* tp_str */
+    0,                                  /* tp_getattro */
+    0,                                  /* tp_setattro */
+    0,                                  /* tp_as_buffer */
+    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
+    0,                                  /* tp_doc */
+    0,                                  /* tp_traverse */
+    0,                                  /* tp_clear */
+    0,                                  /* tp_richcompare */
+    0,                                  /* tp_weaklistoffset */
+    0,                                  /* tp_iter */
+    0,                                  /* tp_iternext */
+    0,                                  /* tp_methods */
+    0,                                  /* tp_members */
+    0,                                  /* tp_getset */
+    0,                                  /* tp_base */
+    0,                                  /* tp_dict */
+    0,                                  /* tp_descr_get */
+    0,                                  /* tp_descr_set */
+    0,                                  /* tp_dictoffset */
+    0,                                  /* tp_init */
+    0,                                  /* tp_alloc */
+    jittedcode_new,                     /* tp_new */
+};
+
 /* Use co_lnotab to compute the line number from a bytecode index, addrq.  See
    lnotab_notes.txt for the details of the lnotab representation.
 */

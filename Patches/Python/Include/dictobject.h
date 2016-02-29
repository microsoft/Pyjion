diff --git a/Include/dictobject.h b/Include/dictobject.h
index 80bd330..c2a264f 100644
--- a/Include/dictobject.h
+++ b/Include/dictobject.h
@@ -142,7 +142,7 @@ PyAPI_FUNC(int) _PyDict_DelItemId(PyObject *mp, struct _Py_Identifier *key);
 PyAPI_FUNC(void) _PyDict_DebugMallocStats(FILE *out);
 
 int _PyObjectDict_SetItem(PyTypeObject *tp, PyObject **dictptr, PyObject *name, PyObject *value);
-PyObject *_PyDict_LoadGlobal(PyDictObject *, PyDictObject *, PyObject *);
+PyAPI_FUNC(PyObject *) _PyDict_LoadGlobal(PyDictObject *, PyDictObject *, PyObject *);
 #endif
 
 #ifdef __cplusplus

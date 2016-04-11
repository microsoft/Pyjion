diff --git a/Objects/codeobject.c b/Objects/codeobject.c
index 353f414..93d7ad9 100644
--- a/Objects/codeobject.c
+++ b/Objects/codeobject.c
@@ -152,6 +152,7 @@ PyCode_New(int argcount, int kwonlyargcount,
     co->co_lnotab = lnotab;
     co->co_zombieframe = NULL;
     co->co_weakreflist = NULL;
+    co->co_extra = NULL;
     return co;
 }
 
@@ -370,6 +371,7 @@ code_dealloc(PyCodeObject *co)
     Py_XDECREF(co->co_filename);
     Py_XDECREF(co->co_name);
     Py_XDECREF(co->co_lnotab);
+    Py_XDECREF(co->co_extra);
     if (co->co_cell2arg != NULL)
         PyMem_FREE(co->co_cell2arg);
     if (co->co_zombieframe != NULL)

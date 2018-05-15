#include "ipycomp.h"

const char* LK_ToString(LocalKind localKind) {
	switch (localKind) {
		case LK_Pointer: return "void*";
		case LK_Float: return "double";
		case LK_Int: return "int";
		case LK_Bool: return "bool";
		case LK_Void: return "void";
	}
	return nullptr;
}

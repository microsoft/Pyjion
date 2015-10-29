
/************************************************************************
* Compiler driver code...
*/



// Checks to see if we have a non-zero error code on the stack, and if so,
// branches to the current error handler.  Consumes the error code in the process
void PythonCompiler::emit_int_error_check() {
	auto noErr = emit_define_label();
	emit_int(0);
	emit_branch(BranchEqual, noErr);

	branch_raise();
	emit_mark_label(noErr);
}

void PythonCompiler::emit_int_error_check(int curIndex) {
	auto noErr = emit_define_label();
	emit_int(0);
	emit_branch(BranchEqual, noErr);
	// we need to issue a leave to clear the stack as we may have
	// values on the stack...

	// TODO: We need to release any objects which are on the stack when we take an
	// error.
	branch_raise();
	emit_mark_label(noErr);
}

// Checks to see if we have a null value as the last value on our stack
// indicating an error, and if so, branches to our current error handler.
void PythonCompiler::emit_error_check() {
	auto noErr = emit_define_label();
	emit_dup();
	emit_null();
	emit_branch(BranchNotEqual, noErr);

	emit_pop();
	branch_raise();
	emit_mark_label(noErr);
}

void PythonCompiler::emit_error_check(int curIndex, const char* reason) {
	auto noErr = emit_define_label();
	emit_dup();
	emit_null();
	emit_branch(BranchNotEqual, noErr);
	// we need to issue a leave to clear the stack as we may have
	// values on the stack...

	emit_pop();
	branch_raise();
	emit_mark_label(noErr);
}

Label PythonCompiler::getOffsetLabel(int jumpTo) {
	auto jumpToLabelIter = m_offsetLabels.find(jumpTo);
	Label jumpToLabel;
	if (jumpToLabelIter == m_offsetLabels.end()) {
		m_offsetLabels[jumpTo] = jumpToLabel = emit_define_label();
	}
	else {
		jumpToLabel = jumpToLabelIter->second;
	}
	return jumpToLabel;
}

vector<Label>& PythonCompiler::getRaiseAndFreeLabels(size_t blockId) {
	while (m_raiseAndFree.size() <= blockId) {
		m_raiseAndFree.emplace_back();
	}

	return m_raiseAndFree.data()[blockId];
}

void PythonCompiler::branch_raise() {
	auto ehBlock = get_ehblock();

	auto& entry_stack = ehBlock.Stack;

	// clear any non-object values from the stack up
	// to the stack that owned the block when we entered.
	size_t count = m_stack.size() - entry_stack.size();

	for (size_t i = m_stack.size(); i-- > entry_stack.size();) {
		if (m_stack[i] == STACK_KIND_VALUE) {
			count--;
			emit_pop();
		}
		else {
			break;
		}
	}

	if (count == 0) {
		emit_branch(BranchAlways, ehBlock.Raise);
	}
	else {
		// We don't currently expect to have stacks w/ mixed value and object types...
		// If we hit this then we need to support cleaning those up too.
		for (auto value : m_stack) {
			_ASSERTE(value != STACK_KIND_VALUE);
		}

		auto& labels = getRaiseAndFreeLabels(ehBlock.BlockId);

		for (size_t i = labels.size(); i < count; i++) {
			labels.push_back(emit_define_label());
		}

		emit_branch(BranchAlways, labels[count - 1]);
	}
}

void PythonCompiler::clean_stack_for_reraise() {
	auto ehBlock = get_ehblock();

	auto& entry_stack = ehBlock.Stack;
	size_t count = m_stack.size() - entry_stack.size();

	for (size_t i = m_stack.size(); i-- > entry_stack.size();) {
		decref();
	}
}

void PythonCompiler::fancy_call(int na, int nk, int flags) {
	int n = na + 2 * nk;
#define CALL_FLAG_VAR 1
#define CALL_FLAG_KW 2
	Local varArgs, varKwArgs, map;
	if (flags & CALL_FLAG_KW) {
		// kw args dict is last on the stack, save it....
		varKwArgs = emit_spill();
		dec_stack();
	}

	if (nk != 0) {
		// if we have keywords build the map, and then save them...
		build_map(nk);
		map = emit_spill();
	}

	if (flags & CALL_FLAG_VAR) {
		// then save var args...
		varArgs = emit_spill();
		dec_stack();
	}

	// now we have the normal args (if any), and then the function
	// Build a tuple of the normal args...
	if (na != 0) {
		build_tuple(na);
	}
	else {
		emit_null();
	}

	// If we have keywords load them or null
	if (nk != 0) {
		emit_load_local(map);
	}
	else {
		emit_null();
	}

	// If we have var args load them or null
	if (flags & CALL_FLAG_VAR) {
		emit_load_local(varArgs);
	}
	else {
		emit_null();
	}

	// If we have a kw dict, load it...
	if (flags & CALL_FLAG_KW) {
		emit_load_local(varKwArgs);
	}
	else {
		emit_null();
	}
	dec_stack(); // for the callable
				 // finally emit the call to our helper...

	emit_fancy_call();
	emit_error_check();

	// the result is back...
	inc_stack();
}

void PythonCompiler::build_tuple(size_t argCnt) {
	emit_new_tuple(argCnt);
	if (argCnt != 0) {
		emit_error_check(-1, "new tuple failed");
		emit_tuple_store(argCnt);
		dec_stack(argCnt);
	}
}

void PythonCompiler::build_list(size_t argCnt) {
	emit_new_list(argCnt);
	emit_error_check(-1, "new list failed");
	if (argCnt != 0) {
		emit_list_store(argCnt);
	}
	dec_stack(argCnt);
}


void PythonCompiler::build_set(size_t argCnt) {
	emit_new_set();
	emit_error_check(-1, "new set failed");
	emit_set_store(argCnt);
	dec_stack(argCnt);
}


void PythonCompiler::build_map(size_t  argCnt) {
	emit_new_dict(argCnt);
	emit_error_check(-1, "new dict failed");

	if (argCnt > 0) {
		auto map = emit_spill();
		for (size_t curArg = 0; curArg < argCnt; curArg++) {
			emit_load_local(map, false);

			emit_dict_store();

			dec_stack(2);
			emit_int_error_check(-1);
		}
		emit_load_local(map);
	}
}

void PythonCompiler::make_function(int posdefaults, int kwdefaults, int num_annotations, bool isClosure) {
	emit_new_function();
	dec_stack(2);

	if (isClosure) {
		emit_set_closure();
		dec_stack();
	}
	if (num_annotations > 0 || kwdefaults > 0 || posdefaults > 0) {
		auto func = emit_spill();
		//dec_stack();
		if (num_annotations > 0) {
			// names is on stack, followed by values.
			//PyObject* values, PyObject* names, PyObject* func
			auto names = emit_spill();
			dec_stack();

			// for whatever reason ceval.c has "assert(num_annotations == name_ix+1);", where
			// name_ix is the numbe of elements in the names tuple.  Otherwise num_annotations
			// goes unused!
			// TODO: Stack count isn't quite right here...
			build_tuple(num_annotations - 1);

			emit_load_local(names);
			emit_load_local(func, false);
			emit_set_annotations();

			emit_int_error_check();
		}
		if (kwdefaults > 0) {
			// TODO: If we hit an OOM here then build_map doesn't release the function
			build_map(kwdefaults);
			emit_load_local(func, false);
			emit_set_kw_defaults();
			emit_int_error_check();
		}
		if (posdefaults > 0) {
			build_tuple(posdefaults);
			emit_load_local(func, false);

			emit_set_defaults();
			emit_int_error_check();
		}
		emit_load_local(func);
		//inc_stack();
	}
	inc_stack();
}

void PythonCompiler::dec_stack(size_t size) {
	_ASSERTE(m_stack.size() >= size);
	for (size_t i = 0; i < size; i++) {
		m_stack.pop_back();
	}
}

void PythonCompiler::inc_stack(size_t size, bool kind) {
	for (size_t i = 0; i < size; i++) {
		m_stack.push_back(kind);
	}
}



// Frees our iteration temporary variable which gets allocated when we hit
// a FOR_ITER.  Used when we're breaking from the current loop.
void PythonCompiler::free_iter_local() {
	for (size_t i = m_blockStack.size() - 1; i >= -1; i--) {
		if (m_blockStack[i].Kind == SETUP_LOOP) {
			if (m_blockStack[i].LoopVar.is_valid()) {
				emit_load_local(m_blockStack[i].LoopVar, false);
				emit_pop_top();
				break;
			}
		}
	}
}

// Frees all of the iteration variables in a range. Used when we're
// going to branch to a finally through multiple loops.
void PythonCompiler::free_all_iter_locals(size_t to) {
	for (size_t i = m_blockStack.size() - 1; i != to - 1; i--) {
		if (m_blockStack[i].Kind == SETUP_LOOP) {
			if (m_blockStack[i].LoopVar.is_valid()) {
				emit_load_local(m_blockStack[i].LoopVar, false);
				emit_pop_top();
			}
		}
	}
}

// Frees all of our iteration variables.  Used when we're unwinding the function
// on an exception.
void PythonCompiler::free_iter_locals_on_exception() {
	int loopCount = 0;
	for (size_t i = m_blockStack.size() - 1; i != -1; i--) {
		if (m_blockStack[i].Kind == SETUP_LOOP) {
			if (m_blockStack[i].LoopVar.is_valid()) {
				emit_load_local(m_blockStack[i].LoopVar, false);
				emit_pop_top();
			}
		}
		else {
			break;
		}
	}
}

// Handles POP_JUMP_IF_FALSE/POP_JUMP_IF_TRUE with a possible error value on the stack.
// If the value on the stack is -1, we branch to the current error handler.
// Otherwise branches based if the current value is true/false based upon the current opcode 
void PythonCompiler::branch_or_error(int& i) {
	auto jmpType = m_byteCode[i + 1];
	i++;
	mark_offset_label(i);
	auto oparg = NEXTARG();

	emit_dup();
	emit_int(-1);

	auto noErr = emit_define_label();
	emit_branch(BranchNotEqual, noErr);
	// we need to issue a leave to clear the stack as we may have
	// values on the stack...

	emit_pop();
	branch_raise();
	emit_mark_label(noErr);

	emit_branch(jmpType == POP_JUMP_IF_FALSE ? BranchFalse : BranchTrue, getOffsetLabel(oparg));
	m_offsetStack[oparg] = m_stack;
}

// Handles POP_JUMP_IF_FALSE/POP_JUMP_IF_TRUE with a bool value known to be on the stack.
// Branches based if the current value is true/false based upon the current opcode 
void PythonCompiler::branch(int& i) {
	auto jmpType = m_byteCode[i + 1];
	i++;
	mark_offset_label(i);
	auto oparg = NEXTARG();

	emit_branch(jmpType == POP_JUMP_IF_FALSE ? BranchFalse : BranchTrue, getOffsetLabel(oparg));
	dec_stack();
	m_offsetStack[oparg] = m_stack;
}

JittedCode* PythonCompiler::compile_worker() {
	int oparg;
	Label ok;

	auto raiseLabel = emit_define_label();
	auto reraiseLabel = emit_define_label();

	emit_lasti_init();
	emit_push_frame();

	m_blockStack.push_back(BlockInfo(vector<bool>(), m_blockIds++, raiseLabel, reraiseLabel, Label(), -1, NOP));

	for (int i = 0; i < m_size; i++) {
		auto opcodeIndex = i;
		auto byte = m_byteCode[i];

		// See FOR_ITER for special handling of the offset label
		if (byte != FOR_ITER) {
			mark_offset_label(i);
		}

		auto curStackDepth = m_offsetStack.find(i);
		if (curStackDepth != m_offsetStack.end()) {
			m_stack = curStackDepth->second;
		}

		// update f_lasti
		if (!can_skip_lasti_update(i)) {
			emit_lasti_update(i);
		}

		if (HAS_ARG(byte)) {
			oparg = NEXTARG();
		}
	processOpCode:
		switch (byte) {
		case NOP: break;
		case ROT_TWO: emit_rot_two(); break;
		case ROT_THREE: emit_rot_three(); break;
		case POP_TOP:
			emit_pop_top();
			dec_stack();
			break;
		case DUP_TOP:
			emit_dup_top();
			inc_stack();
			break;
		case DUP_TOP_TWO:
			inc_stack(2);
			emit_dup_top_two();
			break;
		case COMPARE_OP: compare_op(oparg, i, opcodeIndex); break;
		case SETUP_LOOP:
			// offset is relative to end of current instruction
			m_blockStack.push_back(
				BlockInfo(
					m_stack,
					m_blockStack.back().BlockId,
					m_blockStack.back().Raise,
					m_blockStack.back().ReRaise,
					m_blockStack.back().ErrorTarget,
					oparg + i + 1,
					SETUP_LOOP
					)
				);
			break;
		case BREAK_LOOP:
		case CONTINUE_LOOP:
			// if we have finally blocks we need to unwind through them...
			// used in exceptional case...
		{
			bool inFinally = false;
			size_t loopIndex = -1, clearEh = -1;
			for (size_t i = m_blockStack.size() - 1; i != -1; i--) {
				if (m_blockStack[i].Kind == SETUP_LOOP) {
					// we found our loop, we don't need additional processing...
					loopIndex = i;
					break;
				}
				else if (m_blockStack[i].Kind == END_FINALLY || m_blockStack[i].Kind == POP_EXCEPT) {
					if (clearEh == -1) {
						clearEh = i;
					}
				}
				else if (m_blockStack[i].Kind == SETUP_FINALLY) {
					// we need to run the finally before continuing to the loop...
					// That means we need to spill the stack, branch to the finally,
					// run it, and have the finally branch back to our oparg.
					// CPython handles this by pushing the opcode to continue at onto
					// the stack, and then pushing an integer value which indicates END_FINALLY
					// should continue execution.  Our END_FINALLY expects only a single value
					// on the stack, and we also need to preserve any loop variables.
					m_blockStack.data()[i].Flags |= byte == BREAK_LOOP ? BLOCK_BREAKS : BLOCK_CONTINUES;

					if (!inFinally) {
						// only emit the branch to the first finally, subsequent branches
						// to other finallys will be handled by the END_FINALLY code.  But we
						// need to mark those finallys as needing special handling.
						inFinally = true;
						if (clearEh != -1) {
							unwind_eh(m_blockStack[clearEh].ExVars);
						}
						emit_int(byte == BREAK_LOOP ? BLOCK_BREAKS : BLOCK_CONTINUES);
						emit_branch(BranchAlways, m_blockStack[i].ErrorTarget);
						if (byte == CONTINUE_LOOP) {
							m_blockStack.data()[i].ContinueOffset = oparg;
						}
					}
				}
			}

			if (!inFinally) {
				if (clearEh != -1) {
					unwind_eh(m_blockStack[clearEh].ExVars);
				}
				if (byte != CONTINUE_LOOP) {
					free_iter_local();
				}

				if (byte == BREAK_LOOP) {
					assert(loopIndex != -1);
					emit_branch(BranchAlways, getOffsetLabel(m_blockStack[loopIndex].EndOffset));
				}
				else {
					emit_branch(BranchAlways, getOffsetLabel(oparg));
				}
			}

		}
		break;
		case LOAD_BUILD_CLASS:
			emit_load_build_class();
			emit_error_check();
			inc_stack();
			break;
		case JUMP_ABSOLUTE: jump_absolute(oparg); break;
		case JUMP_FORWARD:  jump_absolute(oparg + i + 1); break;
		case JUMP_IF_FALSE_OR_POP:
		case JUMP_IF_TRUE_OR_POP:
			emit_jump_if_or_pop(byte != JUMP_IF_FALSE_OR_POP, oparg);
			m_offsetStack[oparg] = m_stack;
			dec_stack();
			break;
		case POP_JUMP_IF_TRUE:
		case POP_JUMP_IF_FALSE:
			emit_pop_jump_if(byte != POP_JUMP_IF_FALSE, oparg);
			dec_stack();
			m_offsetStack[oparg] = m_stack;
			break;
		case LOAD_NAME:
			emit_load_name(PyTuple_GetItem(m_code->co_names, oparg));
			emit_error_check();
			inc_stack();
			break;
		case STORE_ATTR:
			emit_store_attr(PyTuple_GetItem(m_code->co_names, oparg));
			dec_stack(2);
			emit_int_error_check();
			break;
		case DELETE_ATTR:
			emit_delete_attr(PyTuple_GetItem(m_code->co_names, oparg));
			emit_int_error_check();
			dec_stack();
			break;
		case LOAD_ATTR:
			emit_load_attr(PyTuple_GetItem(m_code->co_names, oparg));
			dec_stack();
			emit_error_check();
			inc_stack();
			break;
		case STORE_GLOBAL:
			emit_store_global(PyTuple_GetItem(m_code->co_names, oparg));
			dec_stack();
			emit_int_error_check();
			break;
		case DELETE_GLOBAL:
			emit_delete_global(PyTuple_GetItem(m_code->co_names, oparg));
			emit_int_error_check();
			break;
		case LOAD_GLOBAL:
			emit_load_global(PyTuple_GetItem(m_code->co_names, oparg));
			emit_error_check();
			inc_stack();
			break;
		case LOAD_CONST: load_const(oparg, opcodeIndex); break;
		case STORE_NAME:
			emit_store_name(PyTuple_GetItem(m_code->co_names, oparg));
			dec_stack();
			emit_int_error_check();
			break;
		case DELETE_NAME:
			emit_delete_name(PyTuple_GetItem(m_code->co_names, oparg));
			emit_int_error_check();
			break;
		case DELETE_FAST:
			emit_delete_fast(oparg, PyTuple_GetItem(m_code->co_varnames, oparg));
			break;
		case STORE_FAST: store_fast(oparg, opcodeIndex); break;
		case LOAD_FAST: load_fast(oparg, opcodeIndex); break;
		case UNPACK_SEQUENCE:
			unpack_sequence(oparg, i);
			break;
		case UNPACK_EX: unpack_ex(oparg, i); break;
		case CALL_FUNCTION_VAR:
		case CALL_FUNCTION_KW:
		case CALL_FUNCTION_VAR_KW:
			fancy_call(oparg & 0xff, (oparg >> 8) & 0xff, (byte - CALL_FUNCTION) & 3);
			break;
		case CALL_FUNCTION:
		{
			size_t argCnt = oparg & 0xff, kwArgCnt = (oparg >> 8) & 0xff;
			if (kwArgCnt == 0) {
				if (!emit_call(argCnt)) {
					build_tuple(argCnt);
					emit_call_with_tuple();
					dec_stack();// function
				}
				else {
					dec_stack(argCnt + 1); // + function
				}
			}
			else {
				emit_call(argCnt, kwArgCnt);
				dec_stack();
			}
			emit_error_check();
			inc_stack();
			break;
		}
		case BUILD_TUPLE:
			build_tuple(oparg);
			inc_stack();
			break;
		case BUILD_LIST:
			build_list(oparg);
			inc_stack();
			break;
		case BUILD_MAP:
			emit_build_map(oparg);
			inc_stack();
			break;
		case STORE_SUBSCR:
			dec_stack(3);
			emit_store_subscr();
			emit_int_error_check();
			break;
		case DELETE_SUBSCR:
			dec_stack(2);
			emit_delete_subscr();
			emit_int_error_check();
			break;
		case BUILD_SLICE:
			dec_stack(oparg);
			if (oparg != 3) {
				emit_null();
			}
			emit_build_slice();
			inc_stack();
			break;
		case BUILD_SET:
			emit_build_set(oparg);
			inc_stack();
			break;
		case UNARY_POSITIVE:
			dec_stack();
			emit_unary_positive();
			emit_error_check();
			inc_stack();
			break;
		case UNARY_NEGATIVE:
			dec_stack();
			emit_unary_negative();
			emit_error_check();
			inc_stack();
			break;
		case UNARY_NOT:
			if (m_byteCode[i + 1] == POP_JUMP_IF_TRUE || m_byteCode[i + 1] == POP_JUMP_IF_FALSE) {
				dec_stack(1);
				emit_unary_not_int();
				branch_or_error(i);
			}
			else {
				dec_stack(1);
				emit_unary_not();
				emit_error_check();
				inc_stack();
			}
			break;
		case UNARY_INVERT:
			dec_stack(1);
			emit_unary_invert();
			emit_error_check();
			inc_stack();
			break;
		case BINARY_SUBSCR:
		case BINARY_ADD:
		case BINARY_TRUE_DIVIDE:
		case BINARY_FLOOR_DIVIDE:
		case BINARY_POWER:
		case BINARY_MODULO:
		case BINARY_MATRIX_MULTIPLY:
		case BINARY_LSHIFT:
		case BINARY_RSHIFT:
		case BINARY_AND:
		case BINARY_XOR:
		case BINARY_OR:
		case BINARY_MULTIPLY:
		case BINARY_SUBTRACT:
		case INPLACE_POWER:
		case INPLACE_MULTIPLY:
		case INPLACE_MATRIX_MULTIPLY:
		case INPLACE_TRUE_DIVIDE:
		case INPLACE_FLOOR_DIVIDE:
		case INPLACE_MODULO:
		case INPLACE_ADD:
		case INPLACE_SUBTRACT:
		case INPLACE_LSHIFT:
		case INPLACE_RSHIFT:
		case INPLACE_AND:
		case INPLACE_XOR:
		case INPLACE_OR:
			if (!m_interp.should_box(opcodeIndex)) {
				auto stackInfo = m_interp.get_stack_info(opcodeIndex);
				auto one = stackInfo[stackInfo.size() - 1];
				auto two = stackInfo[stackInfo.size() - 2];

				// Currently we only optimize floating point numbers..
				if (one.Value->kind() == AVK_Float && two.Value->kind() == AVK_Float) {
					_ASSERTE(m_stack[m_stack.size() - 1] == STACK_KIND_VALUE);
					_ASSERTE(m_stack[m_stack.size() - 2] == STACK_KIND_VALUE);

					dec_stack(2);

					emit_binary_float(byte);

					inc_stack(1, STACK_KIND_VALUE);
					break;
				}
			}
			dec_stack(2);

			emit_binary_object(byte);

			emit_error_check();
			inc_stack();

			break;
		case RETURN_VALUE: return_value(opcodeIndex); break;
		case EXTENDED_ARG:
		{
			byte = m_byteCode[++i];
			int bottomArg = NEXTARG();
			oparg = (oparg << 16) | bottomArg;
			goto processOpCode;
		}
		case MAKE_CLOSURE:
		case MAKE_FUNCTION:
			make_function(oparg & 0xff, (oparg >> 8) & 0xff, (oparg >> 16) & 0x7fff, byte == MAKE_CLOSURE);
			break;
		case LOAD_DEREF:
			emit_load_deref(oparg);
			emit_error_check();
			inc_stack();
			break;
		case STORE_DEREF:
			dec_stack();
			emit_store_deref(oparg);
			break;
		case DELETE_DEREF: emit_delete_deref(oparg); break;
		case LOAD_CLOSURE:
			emit_load_closure(oparg);
			inc_stack();
			break;
		case GET_ITER:
			// GET_ITER can be followed by FOR_ITER, or a CALL_FUNCTION.                
		{
			/*
			bool optFor = false;
			Local loopOpt1, loopOpt2;
			if (m_byteCode[i + 1] == FOR_ITER) {
			for (size_t blockIndex = m_blockStack.size() - 1; blockIndex != (-1); blockIndex--) {
			if (m_blockStack[blockIndex].Kind == SETUP_LOOP) {
			// save our iter variable so we can free it on break, continue, return, and
			// when encountering an exception.
			m_blockStack.data()[blockIndex].LoopOpt1 = loopOpt1 = emit_define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			m_blockStack.data()[blockIndex].LoopOpt2 = loopOpt2 = emit_define_local(Parameter(CORINFO_TYPE_NATIVEINT));
			optFor = true;
			break;
			}
			}
			}
			*/
			/*if (optFor) {
			emit_getiter_opt();
			}
			else*/ {
				emit_getiter();
				dec_stack();
				emit_error_check();
				inc_stack();
			}
		}
		break;
		case FOR_ITER:
		{
			BlockInfo *loopBlock = nullptr;
			for (size_t blockIndex = m_blockStack.size() - 1; blockIndex != -1; blockIndex--) {
				if (m_blockStack[blockIndex].Kind == SETUP_LOOP) {
					// save our iter variable so we can free it on break, continue, return, and
					// when encountering an exception.
					loopBlock = &m_blockStack.data()[blockIndex];
					//loopOpt1 = m_blockStack.data()[blockIndex].LoopOpt1;
					//loopOpt2 = m_blockStack.data()[blockIndex].LoopOpt2;
					//inLoop = true;
					break;
				}
			}
			for_iter(i + oparg + 1, i - 2, loopBlock);
			break;
		}
		case SET_ADD:
			// TODO: Are these stack counts right?
			// We emit_error_check with the set/map/list on the
			// stack, but it's not in the count
			emit_set_add();
			dec_stack(2);
			emit_error_check();
			inc_stack();
			break;
		case MAP_ADD:
			emit_map_add();
			dec_stack(3);
			emit_error_check();
			inc_stack();
			break;
		case LIST_APPEND:
			emit_list_append();
			dec_stack(2);
			emit_error_check();
			inc_stack();
			break;
		case PRINT_EXPR:
			emit_print_expr();
			dec_stack();
			emit_int_error_check();
			break;
		case LOAD_CLASSDEREF:
			emit_load_classderef(oparg);
			emit_error_check();
			inc_stack();
			break;
		case RAISE_VARARGS:
			// do raise (exception, cause)
			// We can be invoked with no args (bare raise), raise exception, or raise w/ exceptoin and cause
			switch (oparg) {
			case 0: emit_null();
			case 1: emit_null();
			case 2:
				dec_stack(oparg);
				// raise exc
				emit_raise_varargs();
				// returns 1 if we're doing a re-raise in which case we don't need
				// to update the traceback.  Otherwise returns 0.
				auto curHandler = get_ehblock();
				if (oparg == 0) {
					emit_branch(BranchFalse, curHandler.Raise);
					emit_branch(BranchAlways, curHandler.ReRaise);
				}
				else {
					// if we have args we'll always return 0...
					emit_pop();
					emit_branch(BranchAlways, curHandler.Raise);
				}
				break;
			}
			break;
		case SETUP_EXCEPT:
		{
			auto handlerLabel = getOffsetLabel(oparg + i + 1);
			auto blockInfo = BlockInfo(m_stack, m_blockIds++, emit_define_label(), emit_define_label(), handlerLabel, oparg + i + 1, SETUP_EXCEPT);
			blockInfo.ExVars = ExceptionVars(
				emit_define_local(true),
				emit_define_local(true),
				emit_define_local(true)
				);
			m_blockStack.push_back(blockInfo);
			m_allHandlers.push_back(blockInfo);
			m_ehInfo.push_back(EhInfo(false));
			vector<bool> newStack = m_stack;
			for (int j = 0; j < 3; j++) {
				newStack.push_back(STACK_KIND_OBJECT);
			}
			m_offsetStack[oparg + i + 1] = newStack;
		}
		break;
		case SETUP_FINALLY:
		{
			auto handlerLabel = getOffsetLabel(oparg + i + 1);
			auto blockInfo = BlockInfo(m_stack, m_blockIds++, emit_define_label(), emit_define_label(), handlerLabel, oparg + i + 1, SETUP_FINALLY);
			blockInfo.ExVars = ExceptionVars(
				emit_define_local(true),
				emit_define_local(true),
				emit_define_local(true)
				);
			m_blockStack.push_back(blockInfo);
			m_allHandlers.push_back(blockInfo);
			m_ehInfo.push_back(EhInfo(true));
		}
		break;
		case POP_EXCEPT:
			emit_pop_except();
			break;
		case POP_BLOCK:
		{
			auto curHandler = m_blockStack.back();
			m_blockStack.pop_back();
			if (curHandler.Kind == SETUP_FINALLY || curHandler.Kind == SETUP_EXCEPT) {
				m_ehInfo.data()[m_ehInfo.size() - 1].Flags = curHandler.Flags;

				// convert block into an END_FINALLY BlockInfo which will
				// dispatch to all of the previous locations...
				auto back = m_blockStack.back();
				auto newBlock = BlockInfo(
					back.Stack,
					back.BlockId,
					back.Raise,		// if we take a nested exception this is where we go to...
					back.ReRaise,
					back.ErrorTarget,
					back.EndOffset,
					curHandler.Kind == SETUP_FINALLY ? END_FINALLY : POP_EXCEPT,
					curHandler.Flags,
					curHandler.ContinueOffset
					);
				newBlock.ExVars = curHandler.ExVars;
				m_blockStack.push_back(newBlock);
			}
		}
		break;
		case END_FINALLY:
		{
			// CPython excepts END_FINALLY can be entered in 1 of 3 ways:
			//	1) With a status code for why the finally is unwinding, indicating a RETURN
			//			or a continue.  In this case there is an extra retval on the stack
			//	2) With an excpetion class which is being raised.  In this case there are 2 extra
			//			values on the stack, the exception value, and the traceback.
			//	3) After the try block has completed normally.  In this case None is on the stack.
			//
			//	That means in CPython this opcode can be branched to with 1 of 3 different stack
			//		depths, and the CLR doesn't like that.  Worse still the rest of the generated
			//		byte code assumes this is true.  For case 2 an except handler includes tests
			//		and pops which remove the 3 values from the class.  For case 3 the byte code
			//		at the end of the finally range includes the opcode to load None.
			//
			//  END_FINALLY can also be encountered w/o a SETUP_FINALLY, as happens when it's used
			//	solely for re-throwing exceptions.

			auto ehInfo = m_ehInfo.back();
			m_ehInfo.pop_back();
			auto exVars = m_blockStack.back().ExVars;
			m_blockStack.pop_back();

			if (ehInfo.IsFinally) {
				int flags = ehInfo.Flags;

				// restore the previous exception...
				unwind_eh(exVars);

				// We're actually ending a finally.  If we're in an exceptional case we
				// need to re-throw, otherwise we need to just continue execution.  Our
				// exception handling code will only push the exception type on in this case.
				auto finallyReason = emit_define_local();
				auto noException = emit_define_label();
				dec_stack();
				emit_store_local(finallyReason);
				emit_load_local(finallyReason, false);
				emit_py_object(Py_None);
				emit_branch(BranchEqual, noException);

				if (flags & BLOCK_BREAKS) {
					for (size_t i = m_blockStack.size() - 1; i != -1; i--) {
						if (m_blockStack[i].Kind == SETUP_LOOP) {
							emit_load_local(finallyReason, false);
							emit_int(BLOCK_BREAKS);
							emit_branch(BranchEqual, getOffsetLabel(m_blockStack[i].EndOffset));
							break;
						}
						else if (m_blockStack[i].Kind == SETUP_FINALLY) {
							// need to dispatch to outer finally...
							emit_load_local(finallyReason, false);
							emit_branch(BranchAlways, m_blockStack[i].ErrorTarget);
							break;
						}
					}
				}

				if (flags & BLOCK_CONTINUES) {
					for (size_t i = m_blockStack.size() - 1; i != -1; i--) {
						if (m_blockStack[i].Kind == SETUP_LOOP) {
							emit_load_local(finallyReason, false);
							emit_int(BLOCK_CONTINUES);
							emit_branch(BranchEqual, getOffsetLabel(m_blockStack[i].ContinueOffset));
							break;
						}
						else if (m_blockStack[i].Kind == SETUP_FINALLY) {
							// need to dispatch to outer finally...
							emit_load_local(finallyReason, false);
							emit_branch(BranchAlways, m_blockStack[i].ErrorTarget);
							break;
						}
					}
				}

				if (flags & BLOCK_RETURNS) {
					auto exceptional = emit_define_label();
					emit_load_local(finallyReason, false);
					emit_int(BLOCK_RETURNS);
					emit_compare_equal();
					emit_branch(BranchFalse, exceptional);

					bool hasOuterFinally = false;
					for (size_t i = m_blockStack.size() - 1; i != -1; i--) {
						if (m_blockStack[i].Kind == SETUP_FINALLY) {
							// need to dispatch to outer finally...
							emit_load_local(finallyReason, false);
							emit_branch(BranchAlways, m_blockStack[i].ErrorTarget);
							hasOuterFinally = true;
							break;
						}
					}
					if (!hasOuterFinally) {
						emit_branch(BranchAlways, m_retLabel);
					}

					emit_mark_label(exceptional);
				}

				// re-raise the exception...
				free_iter_locals_on_exception();

				emit_restore_err(finallyReason);

				auto ehBlock = get_ehblock();

				clean_stack_for_reraise();

				emit_branch(BranchAlways, get_ehblock().ReRaise);

				emit_mark_label(noException);
				emit_free_local(finallyReason);
			}
			else {
				// END_FINALLY is marking the EH rethrow.  The byte code branches
				// around this in the non-exceptional case.

				// If we haven't sent a branch to this END_FINALLY then we have
				// a bare try/except: which handles all exceptions.  In that case
				// we have no values to pop off, and this code will never be invoked
				// anyway.
				if (m_offsetStack.find(i) != m_offsetStack.end()) {
					dec_stack(3);
					free_iter_locals_on_exception();
					emit_restore_err();

					clean_stack_for_reraise();

					emit_branch(BranchAlways, get_ehblock().ReRaise);
				}
			}
		}
		break;

		case YIELD_FROM:
		case YIELD_VALUE:
			//printf("Unsupported opcode: %d (yield related)\r\n", byte);
			//_ASSERT(FALSE);
			return nullptr;

		case IMPORT_NAME:
			emit_import_name(PyTuple_GetItem(m_code->co_names, oparg));
			dec_stack(2);
			emit_error_check();
			inc_stack();
			break;
		case IMPORT_FROM:
			emit_import_from(PyTuple_GetItem(m_code->co_names, oparg));
			emit_error_check();
			inc_stack();
			break;
		case IMPORT_STAR:
			emit_import_star();
			dec_stack(1);
			emit_int_error_check();
			break;
		case SETUP_WITH:
		case WITH_CLEANUP_START:
		case WITH_CLEANUP_FINISH:
		default:
			//printf("Unsupported opcode: %d (with related)\r\n", byte);
			//_ASSERT(FALSE);
			return nullptr;
		}
	}


	// for each exception handler we need to load the exception
	// information onto the stack, and then branch to the correct
	// handler.  When we take an error we'll branch down to this
	// little stub and then back up to the correct handler.
	if (m_allHandlers.size() != 0) {
		for (int i = 0; i < m_allHandlers.size(); i++) {
			auto raiseAndFreeLabels = getRaiseAndFreeLabels(m_allHandlers[i].BlockId);
			for (size_t curFree = raiseAndFreeLabels.size() - 1; curFree != -1; curFree--) {
				emit_mark_label(raiseAndFreeLabels[curFree]);
				decref();
			}

			emit_mark_label(m_allHandlers[i].Raise);

			emit_eh_trace();

			emit_mark_label(m_allHandlers[i].ReRaise);

			emit_prepare_exception(m_allHandlers[i].ExVars.PrevExc, m_allHandlers[i].ExVars.PrevExcVal, m_allHandlers[i].ExVars.PrevTraceback, m_allHandlers[i].Kind != SETUP_FINALLY);
			emit_branch(BranchAlways, m_allHandlers[i].ErrorTarget);
		}
	}

	// label we branch to for error handling when we have no EH handlers, return NULL.
	auto raiseAndFreeLabels = getRaiseAndFreeLabels(m_blockStack[0].BlockId);
	for (size_t curFree = raiseAndFreeLabels.size() - 1; curFree != -1; curFree--) {
		emit_mark_label(raiseAndFreeLabels[curFree]);
		decref();
	}
	emit_mark_label(raiseLabel);
	emit_mark_label(reraiseLabel);

	emit_null();
	auto finalRet = emit_define_label();
	emit_branch(BranchAlways, finalRet);

	emit_mark_label(m_retLabel);
	emit_load_local(m_retValue);

	emit_mark_label(finalRet);
	emit_pop_frame();

	emit_check_function_result();
	emit_ret();

	return emit_compile();
}

JittedCode* PythonCompiler::compile() {
	bool interpreted = m_interp.interpret();
	preprocess();

	return compile_worker();
}

bool PythonCompiler::can_skip_lasti_update(int opcodeIndex) {
	switch (m_byteCode[opcodeIndex]) {
	case DUP_TOP:
	case SETUP_EXCEPT:
	case NOP:
	case ROT_TWO:
	case ROT_THREE:
	case POP_BLOCK:
	case POP_JUMP_IF_FALSE:
	case POP_JUMP_IF_TRUE:
	case POP_TOP:
	case DUP_TOP_TWO:
	case BREAK_LOOP:
	case CONTINUE_LOOP:
	case END_FINALLY:
	case LOAD_CONST:
	case JUMP_FORWARD:
	case STORE_FAST:
		return true;
	}

	return m_interp.can_skip_lasti_update(opcodeIndex);
}

CorInfoType PythonCompiler::to_clr_type(AbstractValueKind kind) {
	switch (kind) {
	case AVK_Float: return CORINFO_TYPE_DOUBLE;
	}
	return CORINFO_TYPE_NATIVEINT;
}

void PythonCompiler::store_fast(int local, int opcodeIndex) {
	if (!m_interp.should_box(opcodeIndex)) {
		auto stackInfo = m_interp.get_stack_info(opcodeIndex);
		auto stackValue = stackInfo[stackInfo.size() - 1];
		// We only optimize floats so far...

		if (stackValue.Value->kind() == AVK_Float) {
			_ASSERTE(m_stack[m_stack.size() - 1] == STACK_KIND_VALUE);

			emit_store_float(local);
			dec_stack();
			return;
		}
	}

	_ASSERTE(m_stack[m_stack.size() - 1] == STACK_KIND_OBJECT);
	emit_store_fast(local);
	dec_stack();
}

void PythonCompiler::load_const(int constIndex, int opcodeIndex) {
	auto constValue = PyTuple_GetItem(m_code->co_consts, constIndex);
	if (!m_interp.should_box(opcodeIndex)) {
		if (PyFloat_CheckExact(constValue)) {
			emit_float(PyFloat_AsDouble(constValue));
			inc_stack(1, STACK_KIND_VALUE);
			return;
		}
	}
	emit_py_object(constValue);
	inc_stack();
}

void PythonCompiler::return_value(int opcodeIndex) {
	if (!m_interp.should_box(opcodeIndex)) {
		// We need to box the value now...
		auto stackInfo = m_interp.get_stack_info(opcodeIndex);
		// We only optimize floats so far...
		if (stackInfo[stackInfo.size() - 1].Value->kind() == AVK_Float) {
			_ASSERTE(m_stack[m_stack.size() - 1] == STACK_KIND_VALUE);

			// we need to convert the returned floating point value back into a boxed float.
			emit_box_float();
		}
	}

	dec_stack();

	emit_store_local(m_retValue);

	size_t clearEh = -1;
	bool inFinally = false;
	for (size_t blockIndex = m_blockStack.size() - 1; blockIndex != (-1); blockIndex--) {
		if (m_blockStack[blockIndex].Kind == SETUP_FINALLY) {
			// we need to run the finally before returning...
			m_blockStack.data()[blockIndex].Flags |= BLOCK_RETURNS;

			if (!inFinally) {
				// Only emit the store and branch to the inner most finally, but
				// we need to mark all finallys as being capable of being returned
				// through.
				inFinally = true;
				if (clearEh != -1) {
					unwind_eh(m_blockStack[clearEh].ExVars);
				}
				free_all_iter_locals(blockIndex);
				emit_int(BLOCK_RETURNS);
				emit_branch(BranchAlways, m_blockStack[blockIndex].ErrorTarget);
			}
		}
		else if (m_blockStack[blockIndex].Kind == POP_EXCEPT || m_blockStack[blockIndex].Kind == END_FINALLY) {
			// we need to restore the previous exception before we return
			if (clearEh == -1) {
				clearEh = blockIndex;
			}
		}
	}

	if (!inFinally) {
		if (clearEh != -1) {
			unwind_eh(m_blockStack[clearEh].ExVars);
		}
		free_all_iter_locals();
		emit_branch(BranchLeave, m_retLabel);
	}
}


void PythonCompiler::unpack_sequence(size_t size, int opcode) {
	auto valueTmp = emit_spill();
	dec_stack();

	auto success = emit_define_label();
	emit_unpack_sequence(valueTmp, m_sequenceLocals[opcode], success, size);

	branch_raise();

	emit_mark_label(success);
	auto fastTmp = emit_spill();

	//while (oparg--) {
	//	item = items[oparg];
	//	Py_INCREF(item);
	//	PUSH(item);
	//}

	auto tmpOpArg = size;
	while (tmpOpArg--) {
		emit_load_local(fastTmp, false);
		emit_load_array(tmpOpArg);
		inc_stack();
	}

	emit_load_local(valueTmp);
	emit_pop_top();

	emit_free_local(fastTmp);
}

void PythonCompiler::for_iter(int loopIndex, int opcodeIndex, BlockInfo *loopInfo) {
	// CPython always generates LOAD_FAST or a GET_ITER before a FOR_ITER.
	// Therefore we know that we always fall into a FOR_ITER when it is
	// initialized, and we branch back to it for the loop condition.  We
	// do this becaues keeping the value on the stack becomes problematic.
	// At the very least it requires that we spill the value out when we
	// are doing a "continue" in a for loop.

	// oparg is where to jump on break
	auto iterValue = emit_spill();
	dec_stack();
	//bool inLoop = false;
	//Local loopOpt1, loopOpt2;
	if (loopInfo != nullptr) {
		loopInfo->LoopVar = iterValue;
	}

	// now that we've saved the value into a temp we can mark the offset
	// label.
	mark_offset_label(opcodeIndex);	// minus 2 removes our oparg

	emit_load_local(iterValue, false);

	// TODO: It'd be nice to inline this...
	auto processValue = emit_define_label();

	emit_for_next(processValue, iterValue);

	emit_int_error_check();

	jump_absolute(loopIndex);

	emit_mark_label(processValue);
	inc_stack();
}

void PythonCompiler::compare_op(int compareType, int& i, int opcodeIndex) {
	switch (compareType) {
	case PyCmp_IS:
	case PyCmp_IS_NOT:
		if (m_byteCode[i + 1] == POP_JUMP_IF_TRUE || m_byteCode[i + 1] == POP_JUMP_IF_FALSE) {
			emit_is_int(compareType != PyCmp_IS);
			dec_stack(); // popped 2, pushed 1
			branch(i);
		}
		else {
			emit_is(compareType != PyCmp_IS);
			dec_stack();
		}
		break;
		//	// TODO: Missing dec refs here...
		//{
		//	Label same = m_il.define_label();
		//	Label done = m_il.define_label();
		//	emit_branch(BranchEqual, same);
		//	m_il.ld_i(compareType == PyCmp_IS ? Py_False : Py_True);
		//	emit_branch(BranchAlways, done);
		//	emit_mark_label(same);
		//	m_il.ld_i(compareType == PyCmp_IS ? Py_True : Py_False);
		//	emit_mark_label(done);
		//	m_il.dup();
		//	incref();
		//}
		//	break;
	case PyCmp_IN:
		if (m_byteCode[i + 1] == POP_JUMP_IF_TRUE || m_byteCode[i + 1] == POP_JUMP_IF_FALSE) {
			emit_in_int();
			dec_stack(2);
			branch_or_error(i);
		}
		else {
			emit_in();
			dec_stack(2);
			emit_error_check();
			inc_stack();
		}
		break;
	case PyCmp_NOT_IN:
		if (m_byteCode[i + 1] == POP_JUMP_IF_TRUE || m_byteCode[i + 1] == POP_JUMP_IF_FALSE) {
			emit_not_in_int();
			dec_stack(2);
			branch_or_error(i);
		}
		else {
			emit_not_in();
			dec_stack(2);
			emit_error_check();
			inc_stack();
		}
		break;
	case PyCmp_EXC_MATCH:
		if (m_byteCode[i + 1] == POP_JUMP_IF_FALSE) {
			emit_compare_exceptions_int();
			dec_stack(2);
			branch_or_error(i);
		}
		else {
			// this will actually not currently be reached due to the way
			// CPython generates code, but is left for completeness.
			emit_compare_exceptions();
			dec_stack(2);
			emit_error_check(i, "compare ex");
			inc_stack();
		}
		break;
	default:
		if (!m_interp.should_box(opcodeIndex)) {
			auto stackInfo = m_interp.get_stack_info(opcodeIndex);
			// We only optimize floats so far...
			if (stackInfo[stackInfo.size() - 1].Value->kind() == AVK_Float &&
				stackInfo[stackInfo.size() - 2].Value->kind() == AVK_Float) {

				_ASSERTE(m_stack[m_stack.size() - 1] == STACK_KIND_VALUE);
				_ASSERTE(m_stack[m_stack.size() - 2] == STACK_KIND_VALUE);

				emit_compare_float(compareType);
				dec_stack();

				if (m_byteCode[i + 1] == POP_JUMP_IF_TRUE || m_byteCode[i + 1] == POP_JUMP_IF_FALSE) {
					branch(i);
				}
				else {
					// push Python bool onto the stack
					emit_box_bool();
				}
				return;
			}
		}

		bool generated = false;
		if (m_byteCode[i + 1] == POP_JUMP_IF_TRUE || m_byteCode[i + 1] == POP_JUMP_IF_FALSE) {
			generated = emit_compare_object_ret_bool(compareType);
			if (generated) {
				dec_stack(2);
				branch_or_error(i);
			}
		}

		if (!generated) {
			emit_compare_object(compareType);
			dec_stack(2);
			emit_error_check();
			inc_stack();
		}
		break;
	}
}

void PythonCompiler::load_fast(int local, int opcodeIndex) {
	if (!m_interp.should_box(opcodeIndex)) {
		// We have an optimized local...
		auto localInfo = m_interp.get_local_info(opcodeIndex, local);
		auto kind = localInfo.ValueInfo.Value->kind();
		// We only optimize floats so far...
		if (kind == AVK_Float) {
			emit_load_float(local);
			inc_stack(1, STACK_KIND_VALUE);
			return;
		}
	}

	bool checkUnbound =
		m_assignmentState.find(local) == m_assignmentState.end() ||
		!m_assignmentState.find(local)->second;

	/* PyObject *value = GETLOCAL(oparg); */
	emit_load_fast(local, checkUnbound);

	inc_stack();
}

void PythonCompiler::unpack_ex(size_t size, int opcode) {
	auto valueTmp = emit_spill();
	auto listTmp = emit_define_local();
	auto remainderTmp = emit_define_local();

	dec_stack();

	emit_unpack_ex(valueTmp, size & 0xff, size >> 8, m_sequenceLocals[opcode], listTmp, remainderTmp);
	// load the iterable, the sizes, and our temporary 
	// storage if we need to iterate over the object, 
	// the list local address, and the remainder address
	// PyObject* seq, size_t leftSize, size_t rightSize, PyObject** tempStorage, PyObject** list, PyObject*** remainder

	emit_error_check(); // TODO: We leak the sequence on failure

	auto fastTmp = emit_spill();

	// load the right hand side...
	auto tmpOpArg = size >> 8;
	while (tmpOpArg--) {
		emit_load_local(remainderTmp, false);
		emit_load_array(tmpOpArg);
		inc_stack();
	}

	// load the list
	emit_load_local(listTmp);
	inc_stack();
	// load the left hand side
	//while (oparg--) {
	//	item = items[oparg];
	//	Py_INCREF(item);
	//	PUSH(item);
	//}

	tmpOpArg = size & 0xff;
	while (tmpOpArg--) {
		emit_load_local(fastTmp, false);
		emit_load_array(tmpOpArg);
		inc_stack();
	}

	emit_load_local(valueTmp);
	emit_pop_top();

	emit_free_local(fastTmp);
	emit_free_local(remainderTmp);
}

void PythonCompiler::jump_absolute(int index) {
	m_offsetStack[index] = m_stack;
	emit_branch(BranchAlways, getOffsetLabel(index));
}

"""Auto-generate a rough portion of absvalue.(h|cpp)."""
import dis
import operator
import pathlib
import types


forward_declaration = """
class {avk_name}Value : public AbstractValue {{
    virtual AbstractValueKind kind();
    virtual AbstractValue* binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other);
    virtual AbstractValue* unary(AbstractSource* selfSources, int op);
    virtual AbstractValue* compare(AbstractSource* selfSources, int op, AbstractValueWithSources& other);
    virtual const char* describe();
}};
"""


class_definition = """
AbstractValueKind {avk_name}Value::kind() {{
    return AVK_{avk_name};
}}

AbstractValue* {avk_name}Value::binary(AbstractSource* selfSources, int op, AbstractValueWithSources& other) {{
    auto other_kind = other.Value->kind();
{binary_return_types}
    return AbstractValue::binary(selfSources, op, other);
}}

AbstractValue* {avk_name}Value::unary(AbstractSource* selfSources, int op) {{
{unary_return_types}
    return AbstractValue::unary(selfSources, op);
}}

AbstractValue* {avk_name}Value::compare(AbstractSource* selfSources, int op, AbstractValueWithSources& other) {{
    auto other_kind = other.Value->kind();
{compare_return_types}
    return AbstractValue::compare(selfSources, op, other);
}}

const char* {avk_name}Value::describe() {{
    return "{description}";
}}
"""


def type_details(example, avk_name, description=None):
    """Collection of details about a Python type."""
    return types.SimpleNamespace(example=example, avk_name=avk_name, description=description or avk_name.lower())


def function_():
    """Example function."""
    pass


known_types = {
    bool: type_details(True, 'Bool'),
    bytes: type_details(b'a', 'Bytes'),
    complex: type_details(2+3j, 'Complex'),
    dict: type_details({0: 1}, 'Dict'),
    float: type_details(3.14, 'Float'),
    type(function_): type_details(function_, 'Function'),
    int: type_details(42, 'Integer', 'int'),
    list: type_details([2], 'List'),
    type(None): type_details(None, 'None'),
    set: type_details({3}, 'Set'),
    slice: type_details(slice(13), 'Slice'),
    str: type_details('a', 'String', 'str'),
    tuple: type_details((4,), 'Tuple'),
}


unary_operations = {
    'UNARY_POSITIVE': operator.pos,
    'UNARY_NEGATIVE': operator.neg,
    'UNARY_NOT': operator.not_,
    'UNARY_INVERT': operator.invert,
    # GET_ITER
    # GET_YIELD_FROM_ITER
}

def unary(type_):
    """Calculate the return type for all unary operations."""
    operation_return_types = {}
    for opcode, operation in unary_operations.items():
        try:
            result_type = type(operation(known_types[type_].example))
        except TypeError:
            continue
        else:
            operation_return_types.setdefault(result_type, []).append(opcode)
    return operation_return_types


binary_operations = {
    'BINARY_POWER': operator.pow,
    'BINARY_MULTIPLY': operator.mul,
    'BINARY_MATRIX_MULTIPLY': operator.matmul,
    'BINARY_FLOOR_DIVIDE': operator.floordiv,
    'BINARY_TRUE_DIVIDE': operator.truediv,
    'BINARY_MODULO': operator.mod,
    'BINARY_ADD': operator.add,
    'BINARY_SUBTRACT': operator.sub,
    'BINARY_SUBSCR': operator.getitem,
    'BINARY_LSHIFT': operator.lshift,
    'BINARY_RSHIFT': operator.rshift,
    'BINARY_AND': operator.and_,
    'BINARY_XOR': operator.xor,
    'BINARY_OR': operator.or_,
    'INPLACE_POWER': operator.ipow,
    'INPLACE_MULTIPLY': operator.imul,
    'INPLACE_MATRIX_MULTIPLY': operator.imatmul,
    'INPLACE_FLOOR_DIVIDE': operator.ifloordiv,
    'INPLACE_TRUE_DIVIDE': operator.itruediv,
    'INPLACE_MODULO': operator.imod,
    'INPLACE_ADD': operator.iadd,
    'INPLACE_SUBTRACT': operator.isub,
    'INPLACE_LSHIFT': operator.ilshift,
    'INPLACE_RSHIFT': operator.irshift,
    'INPLACE_AND': operator.iand,
    'INPLACE_XOR': operator.ixor,
    'INPLACE_OR': operator.ior,
}

compare_operations = {
    'PyCmp_LT': operator.lt,
    'PyCmp_LE': operator.le,
    'PyCmp_GT': operator.gt,
    'PyCmp_GE': operator.ge,
    'PyCmp_IN': lambda x, y: x in y,
    'PyCmp_NOT_IN': lambda x, y: x not in y,
    # Universally defined.
    #'PyCmp_EQ': operator.eq,
    #'PyCmp_NE': operator.ne,
    # Can't be overridden.
    #'PyCmp_IS': operator.is_,
    #'PyCmp_IS_NOT': operator.is_not,
}

def binary(type_, other_type, operations):
    """Calculate the return types for all binary operations (including in-place)."""
    type_example = known_types[type_].example
    other_type_example = known_types[other_type].example
    operation_return_types = {}
    for opcode, operation in operations.items():
        try:
            result_type = type(operation(type_example, other_type_example))
        except (TypeError, IndexError, KeyError):
            continue
        else:
            operation_return_types.setdefault(result_type, []).append(opcode)
    return operation_return_types


def format_opcodes(type_, return_types, *, indent):
    this_type = known_types[type_].avk_name
    output = []
    for return_type, opcodes in sorted(return_types.items(), key=lambda pair: known_types[pair[0]].avk_name):
        for opcode in sorted(opcodes):
            output.append(indent + 'case {}:'.format(opcode))
        output.append(indent + '    return {};'.format(('&' + known_types[return_type].avk_name) if return_type != type_ else 'this'))
    return '\n'.join(output)


def format_unary_opcodes(type_, return_types, *, indent):
    output = []
    opcode_switch = format_opcodes(type_, return_types, indent=indent+'    ')
    if opcode_switch:
        output.append(indent + 'switch (op) {')
        output.append(opcode_switch)
        output.append(indent + '}')
    else:
        output.append('// Does not work with unary operators.')
    return '\n'.join(output)


def format_binary_opcodes(type_, other_type, return_types, *, indent, position):
    output = []
    opcode_switch = format_opcodes(type_, return_types, indent=indent+'        ')
    if opcode_switch:
        output.append(indent + '{}if (other_kind == AVK_{}) {{'.format('' if position == 0 else 'else ', known_types[other_type].avk_name))
        output.append(indent + '    switch (op) {')
        output.append(opcode_switch)
        output.append(indent + '    }')
        output.append(indent + '}')
    return '\n'.join(output)


def main():
    directory = pathlib.Path(__file__).parent
    with open(str(directory/'absvalue.h'), 'w') as file:
        for type_detail in sorted(known_types.values(), key=lambda x: x.avk_name):
            file.write(forward_declaration.format(avk_name=type_detail.avk_name))

    with open(str(directory/'absvalue.cpp'), 'w') as file:
        for type_, type_detail in sorted(known_types.items(), key=lambda pair: pair[1].avk_name):
            unary_return_types = unary(type_)
            unary_opcodes = format_unary_opcodes(type_, unary_return_types, indent='        ')
            binary_opcodes_list = []
            compare_opcodes_list = []
            for position, other_type in enumerate(sorted(known_types, key=lambda x: known_types[x].avk_name)):
                binary_return_types = binary(type_, other_type, binary_operations)
                opcode_if = format_binary_opcodes(type_, other_type, binary_return_types, indent='    ', position=position)
                if opcode_if:
                    binary_opcodes_list.append(opcode_if)
                compare_return_types = binary(type_, other_type, compare_operations)
                opcode_if = format_binary_opcodes(type_, other_type, compare_return_types, indent='    ', position=position)
                if opcode_if:
                    compare_opcodes_list.append(opcode_if)
            file.write(class_definition.format(binary_return_types='\n'.join(binary_opcodes_list), unary_return_types=unary_opcodes,
                                               compare_return_types='\n'.join(compare_opcodes_list), **type_detail.__dict__))


def test():
    for type_, type_detail in known_types.items():
        assert type_ == type(type_detail.example), 'type for example of {} does not match'.format(type_)
    for opcode in unary_operations:
        assert opcode in dis.opmap
    for opcode in binary_operations:
        assert opcode in dis.opmap


if __name__ == '__main__':
    main()

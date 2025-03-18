"""
    Some aux operations with python ast, like calulating ast expessions with specific meaning of identifiers
"""

import compiler


def recurse_calc_ast(node, identifier_func):
    NNARY_OPERATIONS = {  # somehow <and> and <or> are not bitwise operations
        compiler.ast.And: lambda x, y: x and y,
        compiler.ast.Or: lambda x, y: x or y,
        compiler.ast.Bitand: lambda x, y: x & y,
        compiler.ast.Bitor: lambda x, y: x | y,
    }

    BINARY_OPERATIONS = {
        compiler.ast.Add: lambda x, y: x + y,
        compiler.ast.Sub: lambda x, y: x - y,
        compiler.ast.Mul: lambda x, y: x * y,
        compiler.ast.Div: lambda x, y: x / y,
    }
    UNARY_OPERATIONS = {
        compiler.ast.Not: lambda x: not x,
    }

    if isinstance(node, tuple(BINARY_OPERATIONS.keys())):
        left_val = recurse_calc_ast(node.left, identifier_func)
        right_val = recurse_calc_ast(node.right, identifier_func)
        return BINARY_OPERATIONS[node.__class__](left_val, right_val)
    elif isinstance(node, tuple(NNARY_OPERATIONS.keys())):
        vals = map(lambda x: recurse_calc_ast(x, identifier_func), node.nodes)
        result = vals[0]
        for val in vals[1:]:
            result = NNARY_OPERATIONS[node.__class__](result, val)
        return result
    elif isinstance(node, tuple(UNARY_OPERATIONS.keys())):
        val = recurse_calc_ast(node.expr, identifier_func)
        return UNARY_OPERATIONS[node.__class__](val)
    elif isinstance(node, compiler.ast.Const):
        return node.value
    elif isinstance(node, compiler.ast.Name):
        return identifier_func(node.name)
    elif isinstance(node,
                    compiler.ast.Getattr):  # FIXME: such processing of getattr is specific for intmetasearchv2 configs
        identifier = "%s.%s" % (recurse_calc_ast(node.expr, lambda x: str(x)), node.attrname)
        return identifier_func(identifier)
    else:
        raise Exception("Do not know how to process node with class %s" % node.__class__)


def convert_to_ast_and_eval(expr, identifier_func):
    """
        Convert python expression from string <expr> , replacing identifiers with result of identifier func
    """
    node = compiler.parse(expr).getChildren()[1].getChildren()[0].getChildren()[0]
    return recurse_calc_ast(node, identifier_func)

#!/skynet/python/bin/python
"""
    Calculate power ariphmetic expressions. Can use following identifiers in expression:
        - <group name> - power of group . Example "MSK_RESERVED * 2" - double power of MSK_RESERVED
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import compiler

import gencfg
from core.argparse.parser import ArgumentParserExt
from core.db import CURDB
from gaux.aux_ast import recurse_calc_ast


def get_parser():
    parser = ArgumentParserExt(description="Calculate power expression")
    parser.add_argument("-e", "--expression", type=str, required=True,
                        help="Obligatory. Expession to calculate, e.g. 'MSK_RESERVED * 2 - SAS_RESERVED'")

    return parser


def main(options):
    try:
        ast = compiler.parse(options.expression)
    except Exception, e:
        raise Exception("Failed to parse line <%s> as python expression. Error: <%s>" % (options.expression, str(e)))

    ast_expr = ast.getChildren()[1].getChildren()[0].getChildren()[0]

    def identifier_func(s):
        group = CURDB.groups.get_group(s)
        power = sum(map(lambda x: x.power, group.get_instances()))
        return power

    result = recurse_calc_ast(ast_expr, identifier_func)

    return result


def jsmain(d):
    options = get_parser().parse_json(d)
    return main(options)


def print_result(result):
    print result


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    result = main(options)

    print_result(result)

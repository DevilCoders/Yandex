# coding: utf-8

from __future__ import print_function

import sys
import os
import re
from contextlib import closing
import inspect

from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter

import pyparsing

def fix_h(func, maxargs=2):
    nargs = len(inspect.getargspec(func).args)
    if nargs == 1:
        return lambda s, loc, toks: func(toks)
    return func

pyparsing._trim_arity = fix_h

from pyparsing import OneOrMore, CharsNotIn, Word, \
    alphas, alphanums, \
    ZeroOrMore, Keyword, Optional, Literal


def SQLKeyword(k):
    return Keyword(k, caseless=True)


def as_is_action(s):
    return " ".join(s[:-1]) + ";"


def ignore_action(s): # pylint: disable=W0613
    return ""

PROXY_FUNC_TEMPLATE = """
CREATE OR REPLACE FUNCTION %(proxy_func_schema)s.%(name)s (
 %(params)s
) %(returns)s AS $$
    CLUSTER 'rw';
    RUN ON plproxy.select_part(i_uid);
    TARGET %(real_func_schema)s.%(name)s;
$$ LANGUAGE plproxy;
"""

def func_action(func_schema, proxy_func_template, s, loc, toks): # pylint: disable=W0613
    s_params = " ".join(toks.get("params"))
    s_params = s_params.replace(" ,", ",\n")
    reg = re.compile("^", re.M)
    s_params = reg.sub(" "*3, s_params)
    real_func_schema = toks.get("schema")
    returns = " ".join(toks.get("returns"))
    if toks.get("as_table"):
        returns_cols = returns[returns.find("(")+1:returns.rfind(")")]
        returns_cols = [" "*4 + col.strip() for col in returns_cols.split(",")]
        returns = "".join([
            returns[:returns.find("(")+1],
            "\n",
            ",\n".join(returns_cols),
            "\n",
            returns[returns.rfind(')')-1:].lstrip()
        ]).strip()
    args = dict(
        proxy_func_schema=func_schema or real_func_schema,
        real_func_schema=real_func_schema,
        name=toks.get("name"),
        params=s_params,
        returns=returns,
        body_wrap=toks.get('func_body_wrap')
    )
    return (proxy_func_template % args).lstrip()


def make_gramma(func_schema, proxy_func_template): # pylint: disable=R0914
    CREATE = SQLKeyword("CREATE")
    DROP = SQLKeyword("DROP")
    CASCADE = SQLKeyword("CASCADE")
    DEFAULT = SQLKeyword("DEFAULT")
    OR = SQLKeyword("OR")
    REPLACE = SQLKeyword("REPLACE")
    IF = SQLKeyword("IF")
    IN = SQLKeyword("IN")
    OUT = SQLKeyword("OUT")
    INOUT = SQLKeyword("INOUT")
    VARIADIC = SQLKeyword("VARIADIC")
    AS = SQLKeyword("AS")
    ENUM = SQLKeyword("ENUM")
    TYPE = SQLKeyword("TYPE")
    SCHEMA = SQLKeyword("SCHEMA")
    EXISTS = SQLKeyword("EXISTS")
    FUNCTION = SQLKeyword("FUNCTION")
    TABLE = SQLKeyword("TABLE")
    RETURNS = SQLKeyword("RETURNS")
    SETOF = SQLKeyword("SETOF")
    IMMUTABLE = SQLKeyword("IMMUTABLE")
    STRICT = SQLKeyword("STRICT")
    VIOLATIVE = SQLKeyword("VIOLATIVE")
    STABLE = SQLKeyword("STABLE")
    LANGUAGE = SQLKeyword("LANGUAGE")
    WITH = SQLKeyword("WITH")
    WITHOUT = SQLKeyword("WITHOUT")
    TIME = SQLKeyword("TIME")
    ZONE = SQLKeyword("ZONE")
    VARYING = SQLKeyword("VARYING")

    name = Word(alphas, alphanums + "_$.").setName("name")

    create_type_def = (
        CREATE + TYPE +
        name + AS + Optional(ENUM) +
        ZeroOrMore(CharsNotIn(";")) + ";"
    ).setResultsName("create_type").setParseAction(as_is_action)

    create_schema_def = (
        CREATE + SCHEMA + name +
        ZeroOrMore(CharsNotIn(";")) + ";"
    ).setResultsName("create_schema").setParseAction(as_is_action)

    drop_schema_def = (
        DROP + SCHEMA +
        Optional(IF + EXISTS) +
        name + Optional(CASCADE) + ";"
    ).setResultsName("drop_schema").setParseAction(as_is_action)

    comment_def = (
        "--" + ZeroOrMore(CharsNotIn("\n"))
    ).setParseAction(ignore_action)

    type_def_one_word = Word(alphas, alphanums + "_$.%[]")
    type_def_multy_words = Word(alphas, alphanums + "_$.%[]") + \
        OneOrMore(WITH|WITHOUT|TIME|ZONE|VARYING)
    type_def = type_def_multy_words|type_def_one_word

    param_style = IN|OUT|INOUT|VARIADIC
    param_default = (DEFAULT|Literal("=")) + name

    param_def = (
        Optional(param_style) + name + type_def +
        Optional(param_default) +
        Optional(",") + Optional(comment_def)
    )

    function_params = OneOrMore(param_def).setResultsName("params")

    function_begin_def = (
        CREATE + Optional(OR + REPLACE) + FUNCTION
    ).setResultsName("begin")

    schame_name = Word(alphas, alphanums + "_").setResultsName("schema")
    function_name = Word(alphas, alphanums + "_").setResultsName("name")
    function_full_name = Optional(schame_name + Literal(".")) + function_name

    func_language = LANGUAGE + name
    func_modifires = ZeroOrMore(IMMUTABLE|STRICT|VIOLATIVE|STABLE)

    func_body_wrap = (
        Literal('$$')|Literal('$BODY$')
    ).setResultsName('func_body_wrap')

    func_table_columns = OneOrMore(name + type_def + Optional(","))
    func_return_table = (
        TABLE + Literal("(") + func_table_columns + Literal(")")
    ).setResultsName("as_table")
    func_return_defined_type = Optional(SETOF) + type_def

    func_return_type = RETURNS + (func_return_table|func_return_defined_type)

    function_return = (
        func_return_type +
        Optional(func_language) + func_modifires
    ).setResultsName("returns")

    def func_action_with_schema(s, loc, toks):
        return func_action(func_schema, proxy_func_template, s, loc, toks)

    create_function_def = (
        function_begin_def +
        function_full_name +
        Literal("(") + function_params + Literal(")") +
        function_return + Optional(AS) + func_body_wrap
    ).setParseAction(func_action_with_schema)

    other_statement_def = (
        OneOrMore(CharsNotIn(";")) + ";"
    ).setParseAction(ignore_action)


    statement_def = comment_def | \
        create_type_def | \
        create_schema_def | drop_schema_def | \
        create_function_def | \
        other_statement_def

    return OneOrMore(statement_def)


class Parser(object):

    def __init__(self, func_schema, proxy_func_template=None):
        self.gramma = make_gramma(
            func_schema,
            proxy_func_template or PROXY_FUNC_TEMPLATE)
        self.r = []

    def __call__(self, fd, fname):
        self.r.append('-- %s\n' % fname)
        for r in self.gramma.parseFile(fd):
            if r:
                self.r.append(r + "\n")

    def get_result(self):
        return re.sub(r'(?=\b) {2,}', ' ', '\n'.join(self.r))


def main():
    parser = ArgumentParser(
        formatter_class=ArgumentDefaultsHelpFormatter
    )

    parser.add_argument(
        'sources',
        metavar='SOURCE',
        nargs='+',
        help='file or dir with sql types and functions'
    )

    parser.add_argument(
        '-o',
        '--out',
        default='-',
        help='output'
    )
    parser.add_argument(
        '--proxy-template',
        metavar='FILE',
        help='file with proxy function template'
    )
    parser.add_argument(
        '--func-schema',
        dest='func_schema',
        default='',
        help='schema for functions'
    )

    args = parser.parse_args()

    proxy_func_template = PROXY_FUNC_TEMPLATE
    if args.proxy_template:
        with closing(open(args.proxy_template)) as fd:
            proxy_func_template = fd.read()

    parse_file = Parser(args.func_schema, proxy_func_template)

    for src in args.sources:
        if os.path.isdir(src):
            for fname in sorted(os.listdir(src)):
                l_fname = fname.lower()
                if not l_fname.endswith('.sql'):
                    continue
                full_fname = os.path.join(src, fname)
                with closing(open(full_fname)) as fd:
                    parse_file(fd, full_fname)
        else:
            with closing(open(src)) as fd:
                parse_file(fd, src)

    result = parse_file.get_result()

    if args.out == '-':
        sys.stdout.write(result)
    else:
        with closing(open(args.out, 'w')) as fd:
            fd.write(result)


if __name__ == '__main__':
    main()

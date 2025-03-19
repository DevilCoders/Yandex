# coding: utf-8
from StringIO import StringIO as SIO
import pytest

# pylint: disable=E1101

from make_proxy_code import Parser


def make(text):
    parser = Parser('')
    parser(SIO(text), '')
    return parser.get_result()

MULTY_LINE_ENUM = """
CREATE TYPE super.type AS ENUM (
    'super',
    'mega',
    'extra',
    'type'
);
""".strip()
MULTY_LINE_TYPE = """
CREATE TYPE just AS (
    simple int,
    record text
);
""".strip()

MULTY_LINE_TYPE_WIH_NUMERIC = """
CREATE TYPE just.simple AS (
    ORLY text,
    YARLY numeric(20, 0)
);
""".strip()

TYPE_WITH_TIME_ZOME = """
CREATE TYPE foo.bar AS (
    baz time with time zone,
    bazs timestamp without time zone
);""".strip()


def strip_commets(q):
    lines = q.split('\n')
    lines = (l for l in lines if l and not l.startswith('--'))
    return '\n'.join(lines)

@pytest.mark.parametrize(
    "query",
    [
        "DROP SCHEMA IF EXISTS foo;",
        "CREATE SCHEMA foo;",
        "CREATE TYPE super_type AS ENUM ('super', 'type');",
        MULTY_LINE_ENUM,
        MULTY_LINE_TYPE,
        MULTY_LINE_TYPE_WIH_NUMERIC,
        TYPE_WITH_TIME_ZOME
    ]
)
def test_as_is(query):
    assert strip_commets(make(query)) == query

FUNC_BEGIN = "CREATE OR REPLACE FUNCTION"
FUNC_BODY = """AS $$
BEGIN
    RETURN 42;
END;
$$ LANGUAGE plpgsql;"""

def strip_body(q):
    return strip_commets(q[:q.find('$$')])

@pytest.mark.parametrize("func", [
    """
foo.bar (
    x int
) RETURNS smallint""",
    """
foo.bar (
    x int,
    y int
) RETURNS smallint
""",
    """
foo.bar (
    i_uid text
) RETURNS SETOF smallint
""",
    """
foo.get_a_timestamp (
    i_uid bigint
) RETURNS SETOF time WITH TIME ZONE""",
    """
foo.get_tab (
    i_uid int
) RETURNS TABLE (
    foo text,
    bar int,
    baz time WITH TIME ZONE
)""",
    """
foo.get_tab (
    i_uid int
) RETURNS TABLE (
    foo text[],
    bar int[],
    baz time[]
)""",
])
def test_same_function_signature(func):
    full_func = " ".join([FUNC_BEGIN, func.strip(), FUNC_BODY])
    assert strip_body(make(full_func)) == strip_body(full_func)

@pytest.mark.parametrize("removed_part", [
    "STABLE",
    "IMMUTABLE",
    "STRICT",
    "VIOLATIVE",
    "IMMUTABLE STRICT",
    "LANGUAGE plpgsql",
    "LANGUAGE plpgsql IMMUTABLE STRICT",
])
def test_remove(removed_part):
    full_func = " ".join([
        FUNC_BEGIN,
        "test.test (i_uid int) RETURNS SET OF test.test_table%type",
        removed_part,
        FUNC_BODY
    ])
    res = strip_body(make(full_func))
    assert removed_part not in res
    assert removed_part.lower() not in res.lower()

def test_func_body():
    proxy_template = """
PROXY_SCHEMA=%(proxy_func_schema)s
FUNC_NAME=%(name)s
FUNC_SCHEMA=%(real_func_schema)s"""
    parser = Parser('proxy_schema_override', proxy_template)
    parser(SIO("""
CREATE OR REPLACE FUNCTION Simple.IsBetterThanComplex(i_uid int) RETURNS text AS $$
    RETURN 'Now is better than never';
$$ LANGUAGE plgpgsql;
    """), '')
    res = strip_commets(parser.get_result())
    res_lines = [l for l in res.split('\n') if l]
    assert len(res_lines) == 3
    res_dict = dict([l.split('=') for l in res_lines])
    assert res_dict['PROXY_SCHEMA'] == 'proxy_schema_override'
    assert res_dict['FUNC_NAME'] == 'IsBetterThanComplex'
    assert res_dict['FUNC_SCHEMA'] == 'Simple'


"""
Functions steps
"""

from typing import Any, List, NamedTuple

from behave import then, when
from hamcrest import assert_that, described_as, equal_to, has_entry, not_

from cloud.mdb.dbaas_metadb.tests.helpers.connect import connect
from cloud.mdb.dbaas_metadb.tests.helpers.queries import execute_query_in_transaction

Q_GET_FUNCTIONS = """
SELECT oid::regproc AS func_name,
       (SELECT array_agg(nspname || '.' || typname ORDER BY rn)::text[]
          FROM unnest(proargtypes) WITH ORDINALITY AS x(arg_oid, rn)
          JOIN pg_type
            ON (pg_type.oid = arg_oid)
          JOIN pg_namespace
            ON (typnamespace = pg_namespace.oid)) AS func_args
  FROM pg_proc
 WHERE prolang = (
     SELECT oid
       FROM pg_language
      WHERE lanname = 'sql')
   AND pronamespace = (
      SELECT oid
        FROM pg_namespace
       WHERE nspname = %(schema)s)
   AND proretset
   AND provolatile != 'v'
"""


class SQLFunction(NamedTuple):  # pylint: disable=too-few-public-methods
    """
    SQL function
    """

    name: str
    args: List[str]


@when('I get all STABLE or IMMUTABLE SQL functions in schema "{schema:w}"')
def step_get_functions_in_schema(context, schema):
    conn = connect(context.dsn)
    rows = execute_query_in_transaction(conn, Q_GET_FUNCTIONS, dict(schema=schema)).fetch()
    context.sql_functions = [SQLFunction(name=r.func_name, args=(r.func_args or [])) for r in rows]
    conn.close()


TYPE_DEFAULTS = {
    'pg_catalog.text': "''",
    'pg_catalog.int8': '8',
    'pg_catalog.int4': '4',
    'pg_catalog.int2': '2',
}


def explain_function(conn: Any, func: SQLFunction) -> dict:
    """
    Get function plan
    """
    arg_list = []
    for arg_type in func.args:
        arg_list.append(TYPE_DEFAULTS.get(arg_type, 'NULL') + '::' + arg_type)
    ex_q = 'EXPLAIN (FORMAT JSON ) SELECT * FROM {0}({1})'.format(func.name, ",".join(arg_list))
    all_plan_rows = execute_query_in_transaction(conn, ex_q).fetch()
    if len(all_plan_rows) != 1:
        raise RuntimeError(f'Unexpected - plan has more then one row: {all_plan_rows!r}')
    plan_column = all_plan_rows[0][0]
    if not isinstance(plan_column, list) or len(plan_column) != 1:
        # on my samples it should be a something like
        # [{'Plan': {'Alias': 'get_hosts_by_cid',
        #   'Function Name': 'get_hosts_by_cid',
        #   'Node Type': 'Function Scan',
        #   'Parallel Aware': False,
        #   'Plan Rows': 1000,
        #   'Plan Width': 317,
        #   'Startup Cost': 0.25,
        #   'Total Cost': 10.25}}]
        raise RuntimeError(f'Got unexpected plan column f{plan_column!r}. Postgre change it\'s format?')
    return plan_column[0]


@then('all their plans inlines')
def step_check_functions_plans(context):
    conn = connect(context.dsn)
    try:
        for func in context.sql_functions:
            plan = explain_function(conn, func)
            assert_that(
                plan,
                described_as(
                    f'Plan for {func.name} inlines',
                    has_entry('Plan', has_entry('Node Type', not_(equal_to('Function Scan')))),
                ),
            )
    finally:
        conn.close()

"""
Step around plpgsql_check
"""

import yaml
from behave import given, then

from cloud.mdb.dbaas_metadb.tests.helpers.queries import execute_query


@given('initialized linter')
def step_init_linter(context):
    """
    Initialize linter
    """
    with execute_query(context.dsn, 'CREATE EXTENSION IF NOT EXISTS plpgsql_check'):
        pass


@given('linter ignore')
def step_linter_ignores(context):
    context.linter_ignore = yaml.safe_load(context.text)


@then('linter find nothing for functions in schema "{schema:w}"')
def step_lint_schema(context, schema):
    """
    Lint schema
    """
    got_errors = False
    ignore = {}
    if 'linter_ignore' in context:
        ignore = context.linter_ignore

    with execute_query(
        context.dsn,
        """SELECT p.oid AS func_oid,
                  p.proname AS func_name
             FROM pg_catalog.pg_namespace n
             JOIN pg_catalog.pg_proc p ON pronamespace = n.oid
             JOIN pg_catalog.pg_language l ON p.prolang = l.oid
            WHERE n.nspname = %(schema)s
              AND l.lanname = 'plpgsql'
              AND p.prorettype <> 2279
        """,
        dict(schema=schema),
    ) as lint_res:
        for func_row in lint_res.fetch():
            with execute_query(
                context.dsn,
                """
                   SELECT *
                     FROM plpgsql_check_function(%(oid)s, performance_warnings => TRUE) AS msg
                """,
                dict(oid=func_row.func_oid),
            ) as lint_res:
                lint_lines = [r.msg for r in lint_res.fetch() if r.msg not in ignore.get(func_row.func_name, [])]
                if lint_lines:
                    got_errors = True
                    print(func_row.func_name + ':')
                    print('\n'.join(["   - '%s'" % l for l in lint_lines]))

    assert not got_errors, 'Linter return errors'

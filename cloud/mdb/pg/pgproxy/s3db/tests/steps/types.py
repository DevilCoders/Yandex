# coding: utf-8

from behave import given, then

from environment import Connection

import logging
log = logging.getLogger('types')

ignore_type_differences = [
]


@given(u'dbname "{dbname}"')
def step_dbname(context, dbname):
    conn = Connection(context.CONNSTRING[dbname])
    try:
        context.connections[dbname] = conn
    except AttributeError:
        context.connections = {dbname: conn}


@then(u'the following data types are the same')
def step_get_types_definition(context):
    for row in context.table:
        context.type = '.'.join(row.cells)
        q = """
            SELECT attribute_name, data_type, attribute_udt_schema, attribute_udt_name,
                    collation_schema, collation_name
                FROM information_schema.attributes
              WHERE udt_schema = %(schema)s
                AND udt_name = %(name)s
              ORDER BY ordinal_position
            """.format(row['schema'], row['name'])

        results = []
        for conn in context.connections.values():
            res = conn.get(q, schema=row['schema'], name=row['name'])
            assert res.errcode is None, "Getting type definition failed!"
            assert res.records, "Data type {schema}.{name} is not found".format(
                schema=row['schema'], name=row['name']
            )
            results.append(res)

        assert_results_are_the_same(context, results)


@then(u'the following enum types are the same')
def step_get_enum_types_definition(context):
    for row in context.table:
        context.type = '.'.join(row.cells)
        q = """
            SELECT e.enumlabel
              FROM pg_enum e
              JOIN pg_type t ON e.enumtypid = t.oid
              JOIN pg_namespace n ON n.oid = t.typnamespace
              WHERE n.nspname = %(schema)s
                AND t.typname = %(name)s
              ORDER BY e.enumsortorder
            """.format(row['schema'], row['name'])

        results = []
        for conn in context.connections.values():
            res = conn.get(q, schema=row['schema'], name=row['name'])
            assert res.errcode is None, "Getting enum type definition failed!"
            assert res.records, "Enum type {schema}.{name} is not found".format(
                schema=row['schema'], name=row['name']
            )
            results.append(res)

        assert_results_are_the_same(context, results)


def assert_results_are_the_same(context, results):
    assert len(results) > 0, "We expect some definitions!"
    log.info(results)
    prev = None
    for attributes in results:
        if context.type in ignore_type_differences:
            log.info("Skipping type %s as it is in ignore_type_differences list" % context.type)
            continue
        if prev is not None:
            assert attributes == prev, (
                "Definitions of %s are not the same in all DBs" % context.type
            )
        prev = attributes

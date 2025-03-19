"""
Populate table steps
"""

import tempfile

from behave import given, when

from cloud.mdb.dbaas_metadb.tests.helpers.populate import run_populate_table


@given('populate datafile')
def step_populate_datafile(context):
    if not getattr(context, 'populate_datafile', None):
        context.populate_datafile = tempfile.NamedTemporaryFile(suffix='metadb-populate')
    context.populate_datafile.seek(0)
    context.populate_datafile.truncate(0)
    context.populate_datafile.write(context.text.encode('utf-8'))
    context.populate_datafile.flush()


@when('I run populate_table on table "{table}"')
@when('I run populate_table on table "{table}" with key "{key}"')
def step_run_populate_table(context, table, key='name'):
    run_populate_table(table=table, dsn=context.dsn, key=key, file_path=context.populate_datafile.name)

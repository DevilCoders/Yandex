"""
Migrations steps
"""

import difflib
import logging
import os
import subprocess
from typing import Any, NamedTuple
from pathlib import Path

from behave import given, then, when
from hamcrest import assert_that, described_as, equal_to, has_property
from psycopg2.extensions import make_dsn, parse_dsn
import yatest.common

from cloud.mdb.dbaas_metadb.tests.helpers import migrations
from cloud.mdb.dbaas_metadb.tests.helpers.databases import make_dsn_to_database, recreate_database
from cloud.mdb.dbaas_metadb.tests.helpers.locate import define_pg_env, locate_pg_dump, locate_pgmirate
from cloud.mdb.dbaas_metadb.tests.helpers.populate import run_populate_table
from cloud.mdb.dbaas_metadb.tests.helpers.queries import execute_query

log = logging.getLogger(__name__)


def get_root() -> Path:
    """
    Return root of or project
    """
    return Path(__file__).absolute().parent.parent.parent


def fill_default_data(dsn):
    """
    Fill default tables
    """
    data = [
        {
            'table': 'dbaas.cluster_type_pillar',
            'file': 'cluster_type_pillars.json',
            'key': 'type',
        },
        {
            'table': 'dbaas.default_pillar',
            'file': 'default_pillar.json',
            'key': 'id',
        },
        {
            'table': 'dbaas.disk_type',
            'file': 'disk_types.json',
            'key': 'disk_type_id',
        },
        {
            'table': 'dbaas.regions',
            'file': 'regions.json',
            'key': 'region_id',
        },
        {
            'table': 'dbaas.geo',
            'file': 'geos.json',
            'key': 'geo_id',
        },
        {
            'table': 'dbaas.flavor_type',
            'file': 'flavor_types.json',
            'key': 'id',
        },
        {
            'table': 'dbaas.flavors',
            'file': 'flavors.json',
            'key': 'id',
        },
    ]
    for target in data:
        file_path = get_root() / 'tests/data' / target['file']
        run_populate_table(table=target['table'], dsn=dsn, key=target['key'], file_path=file_path)


def _get_default_dsn() -> str:
    """
    get dsn from environment vars that set metadb recipe
    """
    dsn_vars = {'dbname': 'dbaas_metadb'}
    env_vars = {'host': 'METADB_POSTGRESQL_RECIPE_HOST', 'port': 'METADB_POSTGRESQL_RECIPE_PORT'}
    for param, env_var in env_vars.items():
        try:
            dsn_vars[param] = os.environ[env_var]
        except KeyError as exc:
            raise RuntimeError(f"Unable to lookup {param}: {exc} ")
    return make_dsn(**dsn_vars)


@given('default database')
def step_default_database(context):
    context.dsn = _get_default_dsn()
    fill_default_data(context.dsn)


def create_migrate_database() -> str:
    """
    Create metadb_migrated DB. Return DSN to it.
    """
    default_dsn = _get_adm_dsn(_get_default_dsn())
    migrate_dbname = _get_default_dbname(default_dsn) + '_migrated'
    recreate_database(migrate_dbname, default_dsn)
    return make_dsn_to_database(migrate_dbname, default_dsn)


def command_success(out):
    """
    Command is success matcher
    """
    return described_as(out, has_property('returncode', equal_to(0)))


def run_pgmigrate(repo_path: Path, conn: str, target_version: str = 'latest', withouth_code: bool = False) -> None:
    """
    Run pgmigrate
    """

    args = [locate_pgmirate(), 'migrate', '-t', target_version, '-c', conn]
    if withouth_code:
        args += ['-a', '']
    logging.info('Execute %r in %r', args, repo_path)
    cmd = subprocess.Popen(
        args,
        cwd=str(repo_path),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    _, err = cmd.communicate()
    assert_that(cmd, command_success(err.decode('ascii', errors='ignore')))


@given('database at "{version:d}" migration')
def step_make_database_at_migration(context, version):
    """
    create database and migrate it to given migration

    NOTES:

    In arcadia we can't get `code` that belongs to commits near requested migration.
    And we can't simply apply current `code` (cause it works only in trivial cases).

    So your can't use `code` functions before migration to `latest`
    """
    if version == migrations.get_latest_migration_version(get_root()):
        raise NotImplementedError(f'{version} is latest. Use `Given default database` in such case')

    context.dsn = create_migrate_database()
    run_pgmigrate(get_root(), context.dsn, target_version=str(version), withouth_code=True)


@given('default data filled')
def step_dictionaries_filled(context):
    fill_default_data(context.dsn)


@given('fresh default database')
def step_make_empty_database(context):
    recreate_database(_get_default_dbname(context.dsn), _get_adm_dsn(context.dsn))
    run_pgmigrate(get_root(), context.dsn)
    fill_default_data(context.dsn)


def _assert_migration_not_applied_before(context, target_version):
    if target_version == 'latest':
        target_version = migrations.get_latest_migration_version(get_root())
    else:
        target_version = int(target_version)
    applied_version = migrations.get_applied_migration_version(context.dsn)
    assert applied_version < target_version, (
        f'Migration {target_version} already applied to database. Current version is: {applied_version}. '
        'Use `Given database at "..." migration`'
    )


@when('I migrate database to {target_version:w} migration')
def step_migrate_to_version(context, target_version):
    """
    Migrate db to target_version
    """
    _assert_migration_not_applied_before(context, target_version)
    run_pgmigrate(get_root(), context.dsn, target_version=target_version)
    fill_default_data(context.dsn)


def _get_default_dbname(from_dsn: str) -> str:
    return parse_dsn(from_dsn)['dbname']


def _get_single_file_dbname(from_dsn: str) -> str:
    return "single_" + _get_default_dbname(from_dsn)


def _get_adm_dsn(dsn: str) -> str:
    return make_dsn_to_database('postgres', dsn)


def _get_single_file_dsn() -> str:
    default_dsn = _get_default_dsn()
    return make_dsn_to_database(_get_single_file_dbname(default_dsn), default_dsn)


def _current_scenario_output_path(context) -> Path:
    return Path(yatest.common.output_path()) / context.feature.name / context.scenario.name


@given('database from metadb.sql')
def step_db_from_single_file(context):
    default_dsn = _get_default_dsn()
    single_dbname = _get_single_file_dbname(default_dsn)
    recreate_database(single_dbname, _get_adm_dsn(default_dsn))
    new_root = _current_scenario_output_path(context) / 'metadb-sql-single-tree'
    new_root.mkdir(parents=True)
    migrations.make_one_file_tree(new_root, get_root(), 'metadb.sql', ['head/code', 'head/grants'])
    run_pgmigrate(new_root, make_dsn_to_database(single_dbname, default_dsn))


def dump_db(dbname):
    """
    dump dbname DDL
    """
    dsn = parse_dsn(_get_default_dsn())
    cmd_args = [
        locate_pg_dump(),
        '--schema=dbaas',
        '--schema=stats',
        '--schema=code',
        '--schema=grants',
        '--schema-only',
        '--no-owner',
        '--host',
        dsn['host'],
        '--port',
        dsn['port'],
        dbname,
    ]
    cmd = subprocess.Popen(
        cmd_args,
        env=define_pg_env(),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        encoding='utf-8',
    )
    out, err = cmd.communicate()
    assert_that(cmd, command_success(err))
    return out


def sort_dump(ddl: str) -> str:
    """
    Sort SQL dump

    Some statements in pg_dump depends on objects creation time.
    https://a.yandex-team.ru/review/2529761/details#comment-3478021.

    We saw that problem with GRANTS.
    https://paste.yandex-team.ru/9415810
    """

    def sort_key(l):
        if l.startswith('GRANT'):
            return l
        return ''

    ddl_lines = ddl.splitlines()
    ddl_lines.sort(key=sort_key)
    return '\n'.join(ddl_lines)


def dump_db_and_fix_it(dbname: str) -> str:
    return sort_dump(dump_db(dbname))


class DBDump(NamedTuple):
    filename: str
    ddl: str


@when('I dump default database')
def step_dump_defualt_db(context):
    context.default_dump = DBDump(
        filename='migrations.sql',
        ddl=dump_db_and_fix_it(_get_default_dbname(_get_default_dsn())),
    )


@when('I dump database from metadb.sql')
def step_dump_single_db(context):
    context.single_file_dump = DBDump(
        filename='metadb.sql',
        ddl=dump_db_and_fix_it(_get_single_file_dbname(_get_default_dsn())),
    )


def _pretty_diff(from_dump: DBDump, to_dump: DBDump) -> str:
    return '\n'.join(
        difflib.context_diff(
            from_dump.ddl.splitlines(),
            to_dump.ddl.splitlines(),
            fromfile=from_dump.filename,
            tofile=to_dump.filename,
        )
    )


@then('there is no difference between default database and database from metadb.sql')
def step_no_diff_in_ddl(context):
    for dump in [context.default_dump, context.single_file_dump]:
        log.info('saving DDL dump to %s. You can find in test working directory', dump.filename)
        with open(yatest.common.output_path(dump.filename), 'w') as fd:
            fd.write(dump.ddl)

    if context.default_dump.ddl == context.single_file_dump.ddl:
        return

    raise AssertionError(
        """There are differences in DDL.
Since code/* and grants/* moved to migrations/
you should add full CREATE OR REPLACE FUNCTION code.YYY to migration explicitly.

Generated migration can be found in test-results/py3test/testing_out_stuff/V*__generated.sql,
or you can run make in cloud/mdb/dbaas_metadb/tests
"""
        + _pretty_diff(context.default_dump, context.single_file_dump)
    )


class DBFunctionSignature(NamedTuple):
    name: str
    args: str


def _get_schema_functions(dsn: Any, schema: str) -> dict[DBFunctionSignature, str]:
    funcs = {}
    with execute_query(
        dsn,
        """SELECT p.proname AS name,
                  pg_get_function_identity_arguments(p.oid) AS args,
                  pg_get_functiondef(p.oid) AS full_def
             FROM pg_catalog.pg_namespace n
             JOIN pg_catalog.pg_proc p
               ON pronamespace = n.oid
            WHERE n.nspname = %(schema)s""",
        dict(schema=schema),
    ) as res:
        for row in res.fetch():
            log.debug('row = %r', row)
            funcs[
                DBFunctionSignature(
                    name=row.name,
                    args=row.args,
                )
            ] = row.full_def
    return funcs


SCHEMA = 'code'


@then('I generate new code migration')
def step_generate_code_migration(
    context,
):
    migrated_functions = _get_schema_functions(_get_default_dsn(), SCHEMA)
    single_file_functions = _get_schema_functions(_get_single_file_dsn(), SCHEMA)
    if migrated_functions == single_file_functions:
        log.info('No changes in functions')
        return

    next_migration_version = migrations.get_latest_migration_version(get_root()) + 1

    to_create = set(single_file_functions) - set(migrated_functions)
    to_drop = set(migrated_functions) - set(single_file_functions)
    to_change = set()
    for func in single_file_functions:
        if func not in migrated_functions:
            # it's new function
            continue
        if single_file_functions[func] != migrated_functions[func]:
            to_change.add(func)

    new_migration_name = Path(yatest.common.output_path(f'V{next_migration_version:04d}__generated.sql'))
    with open(new_migration_name, 'w') as fd:
        if to_create:
            fd.write('-- New functions\n')
            for func in to_create:
                fd.write(single_file_functions[func].rstrip() + ';\n')

        if to_change:
            fd.write('-- Changed functions\n')
            for func in to_change:
                fd.write(single_file_functions[func].rstrip() + ';\n')

        for func in to_drop:
            fd.write(f'DROP FUNCTION {SCHEMA}.{func.name}({func.args});\n')

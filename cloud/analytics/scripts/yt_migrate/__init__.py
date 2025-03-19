import pkgutil
import os
from importlib import import_module

import click
import yt.wrapper as yt

SCHEMA_VERSION_ATTRIBUTE = '_yt_migrate_schema_version'


class Config:
    def __init__(self, table_prefix, yt_cluster):
        self.table_prefix = table_prefix
        self.yt_cluster = yt_cluster


class Table:
    def __init__(self, name, **kwargs):
        ctx = click.get_current_context()
        self.name = name
        self.table_path = os.path.join(ctx.obj.table_prefix, self.name)
        self.yt_table_path = yt.TablePath(self.table_path, **kwargs)
        self.yt_cluster = ctx.obj.yt_cluster
        self.full_table_path = '{}.`{}`'.format(self.yt_cluster, self.table_path)


def _find_modules_in_path(import_path):
    try:
        module = import_module(import_path)
        for imp, name, is_pkg in pkgutil.iter_modules(path=module.__path__):
            if name[0] not in '_.~':
                yield name
    except ImportError:
        pass


def _get_version(migration_name):
    return migration_name.split('_')[0]


def _get_cluster_from_proxy(proxy):
    return proxy.split('.')[0]


@click.command()
@click.argument('migrations_module')  # Module to look in for migrations.
@click.argument('table_prefix')  # YT Table prefix to use
@click.argument('migration_name', required=False)  # Target migration
@click.option('--proxy', help='YT Proxy url')
def main(migrations_module, table_prefix, migration_name, proxy):
    if proxy is not None:
        yt.config['proxy']['url'] = proxy

    ctx = click.get_current_context()
    ctx.obj = Config(table_prefix, _get_cluster_from_proxy(proxy))

    print("Operations to perform:")
    if migration_name:
        print("  Target specific migration: {}".format(migration_name))
    else:
        print("  Apply all migrations: {}".format(migrations_module))

    print("Running migrations:")

    attribute_path = '{}/@{}'.format(table_prefix, SCHEMA_VERSION_ATTRIBUTE)
    if migration_name == 'zero':
        # Downgrade is not implemented yet, so just delete the attribute
        print("  Reset all migrations in {} on {}...".format(migrations_module, table_prefix), end=' ')
        yt.remove(attribute_path)
        print('OK')
        return

    if yt.exists(attribute_path):
        prefix_version = yt.get(attribute_path)
    else:
        prefix_version = ''

    if migration_name:
        _apply_migration(attribute_path, migration_name, migrations_module, prefix_version)
    else:
        for migration_name in _find_modules_in_path(migrations_module):
            _apply_migration(attribute_path, migration_name, migrations_module, prefix_version)


def _apply_migration(attribute_path, migration_name, migrations_module, prefix_version):
    migration_version = _get_version(migration_name)
    if migration_version > prefix_version:
        print("  Applying {}.{}...".format(migrations_module, migration_name), end=' ')
        migration_module = import_module('{}.{}'.format(migrations_module, migration_name))
        migration_module.upgrade()
        yt.set(attribute_path, migration_version)
        print('OK')

#!/usr/bin/env python3

from click import Choice, group, option, pass_context

from cloud.mdb.cli.common.parameters import JsonParamType
from cloud.mdb.cli.dbaas.commands.backup_group import backup_group
from cloud.mdb.cli.dbaas.commands.cert_group import cert_group
from cloud.mdb.cli.dbaas.commands.cloud_group import cloud_group
from cloud.mdb.cli.dbaas.commands.cluster_group import cluster_group
from cloud.mdb.cli.dbaas.commands.cms_group import cms_group
from cloud.mdb.cli.dbaas.commands.component_group import component_group
from cloud.mdb.cli.dbaas.commands.compute_group import compute_group
from cloud.mdb.cli.dbaas.commands.conductor_group import conductor_group
from cloud.mdb.cli.dbaas.commands.config_group import config_group
from cloud.mdb.cli.dbaas.commands.database_group import database_group
from cloud.mdb.cli.dbaas.commands.deploy_group import deploy_group
from cloud.mdb.cli.dbaas.commands.dictionary_group import dictionary_group
from cloud.mdb.cli.dbaas.commands.dist_group import dist_group
from cloud.mdb.cli.dbaas.commands.folder_group import folder_group
from cloud.mdb.cli.dbaas.commands.format_schema_group import format_schema_group
from cloud.mdb.cli.dbaas.commands.host_group import host_group
from cloud.mdb.cli.dbaas.commands.iam_group import iam_group
from cloud.mdb.cli.dbaas.commands.init_command import init_command
from cloud.mdb.cli.dbaas.commands.juggler_group import juggler_group
from cloud.mdb.cli.dbaas.commands.katan_group import katan_group
from cloud.mdb.cli.dbaas.commands.logs_command import logs_command
from cloud.mdb.cli.dbaas.commands.maintenance_task_group import maintenance_task_group
from cloud.mdb.cli.dbaas.commands.operation_group import operation_group
from cloud.mdb.cli.dbaas.commands.pillar_group import pillar_group
from cloud.mdb.cli.dbaas.commands.porto_group import dom0_group, porto_group
from cloud.mdb.cli.dbaas.commands.profile_group import profile_group
from cloud.mdb.cli.dbaas.commands.psql_command import psql_command
from cloud.mdb.cli.dbaas.commands.quota_group import quota_group
from cloud.mdb.cli.dbaas.commands.resource_preset_group import resource_preset_group
from cloud.mdb.cli.dbaas.commands.shard_group import shard_group
from cloud.mdb.cli.dbaas.commands.solomon_group import solomon_group
from cloud.mdb.cli.dbaas.commands.subcluster_group import subcluster_group
from cloud.mdb.cli.dbaas.commands.task_group import task_group
from cloud.mdb.cli.dbaas.commands.user_group import user_group
from cloud.mdb.cli.dbaas.commands.utils_group import utils_group
from cloud.mdb.cli.dbaas.commands.valid_resource_group import valid_resource_group
from cloud.mdb.cli.dbaas.commands.version_command import version_command
from cloud.mdb.cli.dbaas.commands.vpc_group import vpc_group
from cloud.mdb.cli.dbaas.commands.billing_group import billing_group
from cloud.mdb.cli.dbaas.internal.config import load_config
from cloud.mdb.cli.dbaas.internal.version import check_version


@group(
    context_settings={
        'help_option_names': ['-h', '--help'],
        'terminal_width': 120,
    }
)
@option('-p', '--profile', 'profile_name', help="Configuration profile to use.")
@option(
    '--config-key',
    'config_key_values',
    multiple=True,
    type=(str, JsonParamType()),
    metavar='KEY VALUE',
    help="Override parameter from config file with the specified key and value.",
)
@option('-f', '--format', type=Choice(['json', 'yaml', 'table', 'csv']), help="Output format.")
@option(
    '-n', '--dry-run', is_flag=True, default=False, help='Enable dry run mode and do not perform any modifying actions.'
)
@option('-d', '--debug', is_flag=True, default=False, help="Enable debug output.")
@pass_context
def cli(ctx, profile_name, config_key_values, format, dry_run, debug):
    """DBaaS management tool."""
    check_version()
    ctx.obj = dict(format=format, dry_run=dry_run, debug=debug)
    load_config(ctx, profile_name, config_key_values)


cli.add_command(config_group)
cli.add_command(profile_group)
cli.add_command(cloud_group)
cli.add_command(folder_group)
cli.add_command(cluster_group)
cli.add_command(compute_group)
cli.add_command(subcluster_group)
cli.add_command(shard_group)
cli.add_command(host_group)
cli.add_command(porto_group)
cli.add_command(dom0_group)
cli.add_command(backup_group)
cli.add_command(component_group)
cli.add_command(user_group)
cli.add_command(database_group)
cli.add_command(deploy_group)
cli.add_command(dictionary_group)
cli.add_command(format_schema_group)
cli.add_command(operation_group)
cli.add_command(task_group)
cli.add_command(maintenance_task_group)
cli.add_command(pillar_group)
cli.add_command(resource_preset_group)
cli.add_command(resource_preset_group, 'flavor')
cli.add_command(valid_resource_group)
cli.add_command(utils_group)
cli.add_command(iam_group)
cli.add_command(quota_group)
cli.add_command(solomon_group)
cli.add_command(katan_group)
cli.add_command(dist_group)
cli.add_command(juggler_group)
cli.add_command(cms_group)
cli.add_command(conductor_group)
cli.add_command(cert_group)
cli.add_command(init_command)
cli.add_command(version_command)
cli.add_command(logs_command)
cli.add_command(psql_command)
cli.add_command(vpc_group)
cli.add_command(billing_group)


def main():
    cli()

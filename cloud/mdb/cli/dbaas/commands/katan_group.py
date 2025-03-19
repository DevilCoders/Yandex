from collections import OrderedDict

from click import argument, group, option, pass_context

from cloud.mdb.cli.common.formatting import (
    print_response,
)
from cloud.mdb.cli.common.parameters import JsonParamType, ListParamType
from cloud.mdb.cli.common.prompts import confirm_dangerous_action
from cloud.mdb.cli.common.utils import ensure_at_least_one_option
from cloud.mdb.cli.dbaas.internal.intapi import ClusterType
from cloud.mdb.cli.dbaas.internal.katan import (
    activate_schedule,
    create_schedule,
    delete_schedule,
    get_schedule,
    list_cluster_rollouts,
    list_clusters,
    list_rollouts,
    list_schedules,
    stop_schedule,
    update_cluster,
    update_schedule,
)
from cloud.mdb.cli.dbaas.internal.metadb.common import to_cluster_type


@group("katan")
def katan_group():
    """Katan management commands."""


@katan_group.group("schedule")
def schedule_group():
    """Commands to manage schedules."""


@schedule_group.command("get")
@argument("schedule_id", metavar="ID")
@option("-v", "--verbose", "--stat", "with_stat", is_flag=True, help="Collect schedule stats.")
@pass_context
def get_schedule_command(ctx, schedule_id, with_stat):
    """Get schedule."""
    print_response(ctx, get_schedule(ctx, schedule_id, with_stat))


@schedule_group.command("list")
@option(
    '--name',
    help='Filter objects to output by name. The value can be a pattern to match with'
    ' the syntax of LIKE clause patterns.',
)
@option("-n", "--namespace", help='Filter objects to output by namespace.')
@option('-l', '--limit', default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only schedule IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_schedules_command(ctx, name, namespace, limit, quiet, separator):
    """List schedules."""

    def _table_formatter(schedule):
        return OrderedDict(
            (
                ('id', schedule['id']),
                ('namespace', schedule['namespace']),
                ('name', schedule['name']),
                ('state', schedule['state']),
                ('age', schedule['age']),
                ('max_size', schedule['max_size']),
                ('parallel', schedule['parallel']),
                ('edited_by', schedule['edited_by']),
            )
        )

    schedules = list_schedules(ctx, name, namespace, limit=limit)
    print_response(
        ctx, schedules, default_format='table', table_formatter=_table_formatter, quiet=quiet, separator=separator
    )


@schedule_group.command('create')
@option('--name', required=True, help='Schedule name.')
@option('--namespace', required=True, help='Namespace.')
@option(
    '--match',
    '--match-tags',
    'match_tags',
    required=True,
    type=JsonParamType(),
    help='Tags to match clusters for rollouts, in JSON format.',
)
@option('--commands', required=True, type=JsonParamType(), help='Deploy commands, in JSON format.')
@option('--age', required=True, help='Interval between rollouts.')
@option('--still-age', 'still_age', required=True, help='Interval between failed rollout and the next attempt.')
@option('--max-size', 'max_size', required=True, type=int, help='Max rollout size, in clusters.')
@option(
    '--parallel',
    required=True,
    type=int,
    help='The max number of clusters that can be processed in parallel in one rollout.',
)
@option('--options', type=JsonParamType(), help='Schedule options, in JSON format.')
@pass_context
def create_command(ctx, **kwargs):
    """Create schedule."""
    schedule_id = create_schedule(ctx, **kwargs)
    print(f'Created schedule with ID "{schedule_id}".')


@schedule_group.command('update')
@argument("schedule_id", metavar="ID")
@option('--name', help='Schedule name.')
@option('--namespace', help='Namespace.')
@option(
    '--match',
    '--match-tags',
    'match_tags',
    type=JsonParamType(),
    help='Tags to match clusters for rollouts, in JSON format.',
)
@option('--commands', type=JsonParamType(), help='Deploy commands, in JSON format.')
@option('--age', help='Interval between rollouts.')
@option('--still-age', 'still_age', help='Interval between failed rollout and the next attempt.')
@option('--max-size', 'max_size', type=int, help='Max rollout size, in clusters.')
@option('--parallel', type=int, help='The max number of clusters that can be processed in parallel in one rollout.')
@option('--options', type=JsonParamType(), help='Schedule options, in JSON format.')
@pass_context
def update_command(ctx, schedule_id, **kwargs):
    """Update schedule."""
    ensure_at_least_one_option(
        ctx,
        kwargs,
        'At least one of --name, --namespace, --match, --commands, --options, --age, --still-age, --max-size or'
        ' --parallel option must be specified.',
    )

    update_schedule(ctx, schedule_id, **kwargs)
    print(f'Schedule "{schedule_id}" updated.')


@schedule_group.command("activate")
@argument("schedule_id", metavar="ID")
@pass_context
def activate_schedule_command(ctx, schedule_id):
    """Activate schedule."""
    activate_schedule(ctx, schedule_id)
    print(f'Schedule "{schedule_id}" activated.')


@schedule_group.command("stop")
@argument("schedule_id", metavar="ID")
@pass_context
def stop_schedule_command(ctx, schedule_id):
    """Stop schedule."""
    stop_schedule(ctx, schedule_id)
    print(f'Schedule "{schedule_id}" stopped.')


@schedule_group.command('delete')
@argument("schedule_id", metavar="ID")
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def delete_schedule_command(ctx, schedule_id, force):
    """Delete schedule."""
    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    delete_schedule(ctx, schedule_id)
    print(f'Schedule "{schedule_id}" deleted.')


@katan_group.group("cluster")
def cluster_group():
    """Commands to manage clusters."""
    pass


@cluster_group.command("list")
@option(
    '-c',
    '--cluster',
    '--clusters',
    'cluster_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several cluster IDs. Multiple values can be specified through a comma.',
)
@option(
    '-t',
    '--type',
    '--cluster-type',
    '--cluster-types',
    'cluster_types',
    type=ListParamType(ClusterType()),
    help='Filter objects to output by one or several cluster types. Multiple values can be specified through a comma.',
)
@option('-l', '--limit', default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only schedule IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_clusters_command(ctx, cluster_ids, cluster_types, limit, quiet, separator):
    """List clusters."""

    def _table_formatter(cluster):
        tags = cluster['tags']
        meta = tags['meta']
        return OrderedDict(
            (
                ('id', cluster['id']),
                ('source', tags['source']),
                ('env', meta.get('env')),
                ('rev', meta.get('rev')),
                ('type', to_cluster_type(meta.get('type'))),
                ('status', meta.get('status')),
                ('imported_at', cluster['imported_at']),
                ('auto_update', cluster['auto_update']),
            )
        )

    clusters = list_clusters(ctx, cluster_ids=cluster_ids, cluster_types=cluster_types, limit=limit)
    print_response(
        ctx, clusters, default_format='table', table_formatter=_table_formatter, quiet=quiet, separator=separator
    )


@cluster_group.command("enable-rollouts")
@argument("cluster_id", metavar="ID")
@pass_context
def enable_rollouts_command(ctx, cluster_id):
    """Enable rollouts for the cluster."""
    update_cluster(ctx, cluster_id, auto_update=True)
    print(f'Rollouts for cluster "{cluster_id}" enabled.')


@cluster_group.command("disable-rollouts")
@argument("cluster_id", metavar="ID")
@pass_context
def disable_rollouts_command(ctx, cluster_id):
    """Disable rollouts for the cluster."""
    update_cluster(ctx, cluster_id, auto_update=False)
    print(f'Rollouts for cluster "{cluster_id}" disabled.')


@katan_group.group("rollout")
def rollout_group():
    """Commands to management rollouts."""
    pass


@rollout_group.command("list")
@option('--schedule', 'schedule_id', help='Filter objects to output by schedule ID.')
@option('-l', '--limit', default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only schedule IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_rollouts_command(ctx, quiet, separator, **kwargs):
    """List rollouts."""

    def _table_formatter(rollout):
        return OrderedDict(
            (
                ('id', rollout['id']),
                ('created_at', rollout['created_at']),
                ('started_at', rollout['started_at']),
                ('finished_at', rollout['finished_at']),
                ('created_by', rollout['id']),
                ('schedule_id', rollout['schedule_id']),
                ('schedule_name', rollout['schedule_name']),
            )
        )

    rollouts = list_rollouts(ctx, **kwargs)
    print_response(
        ctx, rollouts, default_format='table', table_formatter=_table_formatter, quiet=quiet, separator=separator
    )


@katan_group.group("cluster-rollout")
def cluster_rollout_group():
    """Commands to manage cluster rollouts."""
    pass


@cluster_rollout_group.command("list")
@option('--schedule', 'schedule_id', help='Filter objects to output by schedule ID.')
@option('--rollout', 'rollout_id', help='Filter objects to output by rollout ID.')
@option(
    '-c',
    '--cluster',
    '--clusters',
    'cluster_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several cluster IDs. Multiple values can be specified through a comma.',
)
@option(
    '-t',
    '--type',
    '--cluster-type',
    '--cluster-types',
    'cluster_types',
    type=ListParamType(ClusterType()),
    help='Filter objects to output by one or several cluster types. Multiple values can be specified through a comma.',
)
@option('--state')
@option('-l', '--limit', default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only schedule IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_cluster_rollouts_command(ctx, quiet, separator, **kwargs):
    """List rollouts."""

    def _table_formatter(rollout):
        return OrderedDict(
            (
                ('rollout_id', rollout['rollout_id']),
                ('cluster_id', rollout['cluster_id']),
                ('state', rollout['state']),
                ('updated_at', rollout['updated_at']),
                ('comment', rollout['comment']),
            )
        )

    rollouts = list_cluster_rollouts(ctx, **kwargs)
    print_response(
        ctx, rollouts, default_format='table', table_formatter=_table_formatter, quiet=quiet, separator=separator
    )

from collections import OrderedDict
from datetime import datetime

from click import argument, ClickException, group, option, pass_context

from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.common.parameters import DateTimeParamType, ListParamType
from cloud.mdb.cli.dbaas.internal import juggler
from cloud.mdb.cli.dbaas.internal.metadb.host import get_hosts


@group('juggler')
def juggler_group():
    """Juggler management commands."""
    pass


@juggler_group.group('downtime')
def downtime_group():
    """Commands to manage downtimes."""
    pass


@downtime_group.command('get')
@argument('downtime_id', metavar='ID')
@pass_context
def get_downtime_command(ctx, downtime_id):
    """Get downtime."""
    print_response(ctx, juggler.get_downtime(ctx, downtime_id))


@downtime_group.command('list')
@option('--user')
@option('--namespace')
@option('--service')
@option('-H', '--host', 'hostname')
@option('-l', '--limit', default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only downtime IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_downtimes_command(ctx, quiet, separator, **kwargs):
    """List downtimes."""

    def _table_formatter(downtime):
        hosts = []
        services = []
        namespaces = []
        tags = []
        for filter in downtime['filters']:
            host = filter.get('host')
            if host:
                hosts.append(host)

            service = filter.get('service')
            if service:
                services.append(service)

            namespace = filter.get('namespace')
            if namespace:
                namespaces.append(namespace)

            tags.extend(filter['tags'])

        return OrderedDict(
            (
                ('ID', downtime['id']),
                ('start time', downtime['start_time']),
                ('end time', downtime['end_time']),
                ('hosts', '\n'.join(hosts)),
                ('services', '\n'.join(services)),
                ('namespaces', '\n'.join(namespaces)),
                ('tags', '\n'.join(tags)),
            )
        )

    print_response(
        ctx,
        juggler.get_downtimes(ctx, **kwargs),
        default_format='table',
        table_formatter=_table_formatter,
        quiet=quiet,
        separator=separator,
    )


@downtime_group.command('create')
@argument('end_time', type=DateTimeParamType())
@option('-H', '--host', '--hosts', 'hostnames', type=ListParamType(), help="Hosts to set downtime for.")
@option(
    '-c',
    '--cluster',
    '--clusters',
    'cluster_ids',
    type=ListParamType(),
    help="Clusters to set downtime for. Firstly it resolves to the list of hosts.",
)
@option('--service')
@option('--namespace')
@option('--description')
@pass_context
def create_downtime_command(ctx, hostnames, cluster_ids, end_time, service, namespace, description):
    """Create downtime."""
    if not hostnames and not cluster_ids:
        ctx.fail('Either --hosts or --clusters option must be specified.')
    if hostnames and cluster_ids:
        ctx.fail('Only one of --hosts or --clusters option can be specified.')

    if cluster_ids:
        hosts = get_hosts(ctx, cluster_ids=cluster_ids)
        hostnames = [host['fqdn'] for host in hosts]

    downtime_ids = juggler.create_downtimes(
        ctx, hostnames=hostnames, service=service, namespace=namespace, end_time=end_time, description=description
    )
    print('\n'.join(downtime_ids))


@downtime_group.command('delete')
@argument('downtime_ids', metavar='IDS', type=ListParamType())
@pass_context
def delete_downtime_command(ctx, downtime_ids):
    """Delete one or several downtimes."""
    juggler.delete_downtimes(ctx, downtime_ids)
    print('Downtimes deleted')


@juggler_group.group('check')
def check_group():
    """Commands to manage checks."""
    pass


@check_group.command('get')
@argument('hostname', metavar='HOST')
@argument('service')
@pass_context
def get_checks_command(ctx, hostname, service):
    """Get check."""
    print_response(ctx, juggler.get_check(ctx, hostname=hostname, service=service))


@check_group.command('list')
@option('--namespace')
@option('-H', '--host', 'hostname')
@option('--service')
@option('-q', '--quiet', is_flag=True, help='Output only check IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@option('-l', '--limit', default=1000, help='Limit the max number of objects in the output.')
@pass_context
def list_checks_command(ctx, namespace, hostname, service, quiet, separator, limit):
    """List checks."""

    def _table_formatter(check):
        return OrderedDict(
            (
                ('host', check['host']),
                ('service', check['service']),
                ('namespace', check['namespace']),
            )
        )

    print_response(
        ctx,
        juggler.get_checks(ctx, namespace=namespace, hostname=hostname, service=service),
        default_format='table',
        table_formatter=_table_formatter,
        quiet=quiet,
        separator=separator,
        limit=limit,
    )


@check_group.command('delete')
@option('--namespace')
@option('-H', '--host', 'hostname')
@option('--service')
@pass_context
def delete_checks_command(ctx, namespace, hostname, service):
    """Delete one or several checks."""
    juggler.delete_checks(ctx, namespace=namespace, hostname=hostname, service=service)
    print('Checks deleted')


@juggler_group.group('event')
def event_group():
    """Commands to manage raw events."""
    pass


@event_group.command('list')
@option('-H', '--host', 'hostname')
@option('--service')
@option('-q', '--quiet', is_flag=True, help='Output only check IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@option('-l', '--limit', default=1000, help='Limit the max number of objects in the output.')
@pass_context
def list_events_command(ctx, hostname, service, quiet, separator, limit):
    """List events."""
    if not hostname:
        raise ClickException('--host option must be specified.')

    def _table_formatter(event):
        return OrderedDict(
            (
                ('host', event['host']),
                ('service', event['service']),
                ('status', event['status']),
                ('received_time', datetime.fromtimestamp(event["received_time"])),
            )
        )

    print_response(
        ctx,
        juggler.get_events(ctx, hostname=hostname, service=service),
        default_format='table',
        table_formatter=_table_formatter,
        quiet=quiet,
        separator=separator,
        limit=limit,
    )

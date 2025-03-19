from collections import OrderedDict

from click import argument, ClickException, group, option, pass_context

from cloud.mdb.cli.common.parameters import FieldsParamType
from cloud.mdb.cli.dbaas.internal import deploy
from cloud.mdb.cli.common.formatting import format_duration, print_response
from cloud.mdb.cli.common.prompts import confirm_dangerous_action


@group('deploy')
def deploy_group():
    """Deploy management commands."""
    pass


@deploy_group.group('group')
def dg_group():
    """Commands to manage deploy groups."""
    pass


@dg_group.command('get')
@argument('group_name', metavar='NAME')
@pass_context
def get_deploy_group_command(ctx, group_name):
    """Get deploy group."""
    print_response(ctx, deploy.get_deploy_group(ctx, group_name))


@dg_group.command('list')
@option('-l', '--limit', default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only deploy group IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_deploy_groups_command(ctx, limit, quiet, separator):
    """List deploy groups."""

    def _table_formatter(group):
        return OrderedDict(
            (
                ('ID', group['id']),
                ('name', group['name']),
            )
        )

    print_response(
        ctx,
        deploy.get_deploy_groups(ctx, limit=limit),
        default_format='table',
        table_formatter=_table_formatter,
        quiet=quiet,
        separator=separator,
    )


@dg_group.command('create')
@argument('group_name', metavar='NAME')
@pass_context
def create_deploy_group_command(ctx, group_name):
    """Create deploy group."""
    print_response(ctx, deploy.create_deploy_group(ctx, group_name))


@deploy_group.group('master')
def master_group():
    """Commands to manage masters."""
    pass


@master_group.command('get')
@argument('fqdn')
@pass_context
def get_master_command(ctx, fqdn):
    """Get master."""
    print_response(ctx, deploy.get_master(ctx, fqdn))


@master_group.command('list')
@option('-l', '--limit', default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only master IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_masters_command(ctx, limit, quiet, separator):
    """List masters."""

    def _table_formatter(master):
        return OrderedDict(
            (
                ('fqdn', master['fqdn']),
                ('group', master['group']),
                ('created', master['createdAt']),
                ('alive', master.get('isAlive', False)),
                ('checked', master['aliveCheckAt']),
                ('open', master['isOpen']),
            )
        )

    print_response(
        ctx,
        deploy.get_masters(ctx, limit=limit),
        default_format='table',
        table_formatter=_table_formatter,
        quiet=quiet,
        separator=separator,
    )


@deploy_group.group('minion')
def minion_group():
    """Commands to manage minions."""
    pass


@minion_group.command('get')
@argument('fqdn')
@pass_context
def get_minion_command(ctx, fqdn):
    """Get minion."""
    print_response(ctx, deploy.get_minion(ctx, fqdn))


@minion_group.command('list')
@option('-l', '--limit', default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only minion IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_minions_command(ctx, limit, quiet, separator):
    """List minions."""

    def _table_formatter(minion):
        return OrderedDict(
            (
                ('fqdn', minion['fqdn']),
                ('group', minion['group']),
                ('created', minion['createdAt']),
                ('updated', minion['updatedAt']),
                ('registered', minion.get('registered', False)),
                ('autoreassign', minion.get('autoReassign', False)),
                ('deleted', minion.get('deleted', False)),
            )
        )

    print_response(
        ctx,
        deploy.get_minions(ctx, limit=limit),
        default_format='table',
        table_formatter=_table_formatter,
        quiet=quiet,
        separator=separator,
    )


@minion_group.command('upsert')
@argument('fqdn')
@option('--group', '--group-name', 'group_name')
@option('--autoreassign', 'autoreassign', type=bool)
@pass_context
def upsert_minion_command(ctx, fqdn, group_name, autoreassign):
    """Create new minion or update the existing one."""
    print_response(ctx, deploy.upsert_minion(ctx, fqdn, deploy_group=group_name, autoreassign=autoreassign))


@minion_group.command('unregister')
@argument('fqdn')
@pass_context
def unregister_minion_command(ctx, fqdn):
    """Unregister minion."""
    print_response(ctx, deploy.unregister_minion(ctx, fqdn))


@minion_group.command('delete')
@argument('fqdn')
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def delete_minion_command(ctx, fqdn, force):
    """Delete minion."""
    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    deploy.delete_minion(ctx, fqdn)
    print(f'Minion "{fqdn}" deleted.')


@deploy_group.group('shipment')
def shipment_group():
    """Commands to manage shipments."""
    pass


@shipment_group.command('get')
@argument('shipment_id', metavar='ID')
@pass_context
def get_shipment_command(ctx, shipment_id):
    """Get shipment."""
    shipment = deploy.get_shipment(ctx, shipment_id)
    commands = deploy.get_commands(ctx, shipment_id=shipment_id, sort_order='asc')
    jobs = deploy.get_jobs(ctx, shipment_id=shipment_id, sort_order='asc')

    print_response(ctx, _format_shipment(ctx, shipment, commands, jobs))


@shipment_group.command('list')
@option('-H', '--host', '--fqdn', 'fqdn')
@option('-l', '--limit', default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only shipment IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_shipments_command(ctx, fqdn, limit, quiet, separator):
    """List shipments."""

    def _table_formatter(shipment):
        commands = [item['command'] for item in shipment['commands']]
        return OrderedDict(
            (
                ('id', shipment['id']),
                ('hosts', '\n'.join(shipment['hosts'])),
                ('commands', '\n'.join(commands)),
                ('created', shipment['created']),
                ('updated', shipment['updated']),
                ('status', shipment['status']),
            )
        )

    shipments = [_format_shipment(ctx, shipment) for shipment in deploy.get_shipments(ctx, fqdn=fqdn, limit=limit)]

    print_response(
        ctx,
        shipments,
        default_format='table',
        table_formatter=_table_formatter,
        quiet=quiet,
        separator=separator,
    )


@deploy_group.group('command')
def command_group():
    """Commands to manage deploy commands."""
    pass


@command_group.command('get')
@argument('command_id', metavar='ID')
@pass_context
def get_command_command(ctx, command_id):
    """Get deploy command."""
    command = deploy.get_command(ctx, command_id)
    jobs = deploy.get_jobs(ctx, shipment_id=command['shipmentID'], sort_order='asc')
    print_response(ctx, _format_command(ctx, command, jobs=jobs))


@command_group.command('list')
@option('--shipment', 'shipment_id')
@option(
    '--fields',
    type=FieldsParamType(),
    default='id,host,command,created,updated,status,shipment_id',
    help='Fields to output. The value "all" equals to all available fields.'
    ' Default is "id,host,command,created,updated,status,shipment_id".',
)
@option('-l', '--limit', default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only command IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_commands_command(ctx, fields, quiet, separator, **kwargs):
    """List deploy commands."""
    commands = [_format_command(ctx, command) for command in deploy.get_commands(ctx, **kwargs)]
    print_response(
        ctx,
        commands,
        default_format='table',
        fields=fields,
        quiet=quiet,
        separator=separator,
    )


@deploy_group.group('job')
def job_group():
    """Commands to manage deploy jobs."""
    pass


@job_group.command('get')
@argument('job_id', metavar='ID')
@option(
    '--failed',
    'failed',
    is_flag=True,
    default=False,
    help='Output only failed job result states.',
)
@option(
    '--all',
    'all',
    is_flag=True,
    default=False,
    help='Output all job result states and do not exclude ones without changes or with requisite errors.',
)
@pass_context
def get_job_command(ctx, job_id, failed, all):
    """Get job."""
    if failed and all:
        raise ClickException('Options --failed and --all cannot be used together.')

    exclude_unchanged_states = False if all else True
    exclude_requisite_errors = False if all else True
    exclude_succeeded_states = True if failed else False

    job = deploy.get_job(ctx, job_id)
    command = deploy.get_command(ctx, job['commandID'])
    job_result = _last_result(deploy.get_job_results(ctx, fqdn=command['fqdn'], ext_job_id=job['extId']))

    result = _format_job(
        ctx,
        job,
        command=command,
        job_result=job_result,
        exclude_unchanged_states=exclude_unchanged_states,
        exclude_succeeded_states=exclude_succeeded_states,
        exclude_requisite_errors=exclude_requisite_errors,
    )

    print_response(ctx, result)


@job_group.command('list')
@option('--shipment', 'shipment_id')
@option(
    '--fields',
    type=FieldsParamType(),
    default='id,external_id,created,updated,status,command_id',
    help='Fields to output. The value "all" equals to all available fields.'
    ' Default is "id,external_id,created,updated,status,command_id".',
)
@option('-l', '--limit', default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only job IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_jobs_command(ctx, fields, quiet, separator, **kwargs):
    """List jobs."""
    jobs = [_format_job(ctx, job) for job in deploy.get_jobs(ctx, **kwargs)]
    print_response(
        ctx,
        jobs,
        default_format='table',
        fields=fields,
        quiet=quiet,
        separator=separator,
    )


def _format_shipment(ctx, shipment, commands=None, jobs=None):
    result = OrderedDict()
    result['id'] = shipment['id']
    result['created'] = shipment['createdAt']
    result['updated'] = shipment['updatedAt']
    result['status'] = shipment['status']
    if 'errorsCount' in shipment:
        result['error_count'] = shipment['errorsCount']
    if 'timeout' in shipment:
        result['timeout'] = format_duration(shipment['timeout'])
    result['parallel'] = shipment['parallel']
    if 'stopOnErrorCount' in shipment:
        result['stop_on_error_count'] = shipment['stopOnErrorCount']
    result['hosts'] = shipment['fqdns']

    if commands:
        result_commands = [_format_command(ctx, command, jobs=jobs, exclude_shipment_id=True) for command in commands]
    else:
        result_commands = []
        for command in shipment['commands']:
            command_string, arguments = _format_command_string(command)
            result_command = OrderedDict()
            result_command['command'] = command_string
            result_command['arguments'] = arguments
            if 'timeout' in command:
                result_command['timeout'] = command['timeout']
            result_commands.append(result_command)
    result['commands'] = result_commands

    return result


def _format_command(ctx, command, *, jobs=None, exclude_shipment_id=False):
    fqdn = command['fqdn']
    command_string, arguments = _format_command_string(command)

    result = OrderedDict()
    result['id'] = command['id']
    result['host'] = fqdn
    result['command'] = command_string
    result['arguments'] = arguments
    if 'timeout' in command:
        result['timeout'] = format_duration(command['timeout'])
    result['created'] = command['createdAt']
    result['updated'] = command['updatedAt']
    result['status'] = command['status']

    if not exclude_shipment_id:
        result['shipment_id'] = command['shipmentID']

    if jobs:
        command_jobs = deploy.filter_jobs(jobs, command_id=command['id'])
        result['jobs'] = _format_command_jobs(ctx, fqdn, command_jobs)

    return result


def _format_command_string(command):
    command_type = command['type']
    if command_type == 'state.sls':
        state = command['arguments'][0]
        command_string = f'{command_type} {state}'
        arguments = command['arguments'][1:]
    else:
        command_string = command_type
        arguments = command['arguments']

    return command_string, arguments


def _format_command_jobs(ctx, fqdn, command_jobs):
    result = []
    for job in command_jobs:
        result_job = OrderedDict()
        result_job['id'] = job['id']
        result_job['status'] = job['status']
        if job['status'] == 'error':
            job_result = _last_result(deploy.get_job_results(ctx, fqdn=fqdn, ext_job_id=job['extId']))
            result_job['result'] = _format_job_result(job_result, exclude_succeeded_states=True)

        result.append(result_job)

    return result


def _format_job(
    ctx,
    job,
    command=None,
    job_result=None,
    exclude_succeeded_states=False,
    exclude_requisite_errors=True,
    exclude_unchanged_states=True,
):
    result = OrderedDict()
    result['id'] = job['id']
    result['external_id'] = job['extId']
    result['created'] = job['createdAt']
    result['updated'] = job['updatedAt']
    result['status'] = job['status']

    if command:
        formatted_command = _format_command(ctx, command)
        result['shipment_id'] = formatted_command['shipment_id']
        result['host'] = formatted_command['host']
        result['command_id'] = formatted_command['id']
        result['command'] = formatted_command['command']
        result['command_arguments'] = formatted_command['arguments']
    else:
        result['command_id'] = job['commandID']

    if job_result:
        result['result'] = _format_job_result(
            job_result,
            exclude_succeeded_states=exclude_succeeded_states,
            exclude_requisite_errors=exclude_requisite_errors,
            exclude_unchanged_states=exclude_unchanged_states,
        )

    return result


def _format_job_result(
    job_result, exclude_succeeded_states=False, exclude_requisite_errors=True, exclude_unchanged_states=True
):
    job_result = job_result['result']
    if not job_result:
        return {}

    states = []
    for state_string, state in job_result['return'].items():
        if not isinstance(state, dict):
            continue

        if exclude_succeeded_states and state.get('result', False):
            continue

        if exclude_requisite_errors and state.get('comment', '').startswith('One or more requisite failed:'):
            continue

        if exclude_unchanged_states and state.get('result', False) and not state.get('changes'):
            continue

        state['state_string'] = state_string

        states.append(state)

    job_return = []
    for state in sorted(states, key=lambda state: state['__run_num__']):
        result_state = OrderedDict()
        result_state['id'] = state['__id__']
        result_state['state'] = _state_function(state['state_string'])
        result_state['name'] = state['name']
        result_state['sls'] = state['__sls__']
        result_state['result'] = state['result']
        result_state['changes'] = state['changes']
        result_state['comment'] = state['comment']
        result_state['started'] = state['start_time']
        result_state['duration'] = format_duration(state['duration'] / 1000)

        job_return.append(result_state)

    return OrderedDict(
        (
            ('success', job_result['success']),
            ('retcode', job_result['retcode']),
            ('return', job_return),
        )
    )


def _state_function(state_string):
    items = state_string.split('_|-')
    return f'{items[0]}.{items[-1]}'


def _last_result(job_results):
    return max(job_results, key=lambda x: x['recordedAt'])

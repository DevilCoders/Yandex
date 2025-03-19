from collections import OrderedDict

from click import argument, group, option, pass_context

from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.dbaas.internal import cms, cmsdb


@group('cms')
def cms_group():
    """CMS commands."""
    pass


@cms_group.group('operation')
def operation_group():
    """Commands to manage CMS operations."""
    pass


@operation_group.command('get')
@argument('operation_id', metavar='ID')
@option('--db', 'db_mode', is_flag=True)
@option('-v', '--verbose', is_flag=True)
@pass_context
def get_operation_command(ctx, operation_id, db_mode, verbose):
    """Get CMS operation."""
    if db_mode:
        _get_operation_db(ctx, operation_id, verbose)
    else:
        _get_operation_api(ctx, operation_id)


def _get_operation_api(ctx, operation_id):
    print_response(ctx, cms.get_operation(ctx, operation_id))


def _get_operation_db(ctx, operation_id, verbose):
    operation = cmsdb.get_operation(ctx, operation_id)
    print_response(ctx, _format_operation(operation, verbose=verbose))


@operation_group.command('list')
@option('--db', 'db_mode', is_flag=True)
@option('-q', '--quiet', is_flag=True, help='Output only operation IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@option('-l', '--limit', type=int, default=1000, help='Limit the max number of objects in the output.')
@pass_context
def list_operations_command(ctx, db_mode, quiet, separator, limit):
    """List CMS operations."""
    if db_mode:
        _list_operations_db(ctx, quiet, separator, limit)
    else:
        _list_operations_api(ctx, quiet, separator, limit)


def _list_operations_api(ctx, quiet, separator, limit):
    print_response(ctx, cms.get_operations(ctx), quiet=quiet, separator=separator, limit=limit)


def _list_operations_db(ctx, quiet, separator, limit):
    def _table_formatter(operation):
        return OrderedDict(
            (
                ('id', operation['operation_id']),
                ('type', operation['operation_type']),
                ('created_at', operation['created_at']),
                ('modified_at', operation['modified_at']),
                ('instance_id', operation['instance_id']),
                ('status', operation['status']),
            )
        )

    operations = cmsdb.get_operations(ctx, limit=limit)
    operations = [_format_operation(operation) for operation in operations]

    print_response(
        ctx,
        operations,
        default_format='table',
        table_formatter=_table_formatter,
        quiet=quiet,
        id_key='operation_id',
        separator=separator,
        limit=limit,
    )


def _format_operation(operation, verbose=None):
    result = OrderedDict(
        (
            ('operation_id', operation['operation_id']),
            ('idempotency_key', operation['idempotency_key']),
            ('operation_type', operation['operation_type']),
            ('status', operation['status']),
            ('comment', operation['comment']),
            ('author', operation['author']),
            ('instance_id', operation['instance_id']),
            ('created_at', operation['created_at']),
            ('modified_at', operation['modified_at']),
            ('explanation', operation['explanation']),
            ('operation_log', operation['operation_log'].splitlines()),
        )
    )

    if verbose:
        result['operation_state'] = operation['operation_state']
        result['executed_step_names'] = operation['executed_step_names']

    return result

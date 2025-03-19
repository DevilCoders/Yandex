import time

from collections import OrderedDict

from click import ClickException

from cloud.mdb.cli.common.formatting import print_response, format_timestamp

from cloud.mdb.cli.dbaas.internal.db import db_query
from cloud.mdb.cli.dbaas.internal.utils import dry_run_mode, get_polling_interval


class NetworkNotFound(ClickException):
    def __init__(self, network_id=None):
        message = f'Network {network_id} not found.' if network_id else 'Network not found.'
        super().__init__(message)


class OperationNotFound(ClickException):
    def __init__(self, operation_d=None):
        message = f'Operation {operation_d} not found.' if operation_d else 'Operation not found.'
        super().__init__(message)


def get_network(ctx, network_id):
    query = """SELECT
        *
    FROM
        vpc.networks v
    WHERE
        network_id = %(network_id)s"""
    network = db_query(
        ctx,
        'vpcdb',
        query,
        network_id=network_id,
        fetch_single=True,
    )
    if not network:
        raise NetworkNotFound(network_id)

    return network


def list_networks(
    ctx,
    project_ids=None,
    region_ids=None,
    statuses=None,
    limit=None,
):
    query = """SELECT
        *
    FROM
        vpc.networks v
    WHERE true
        {% if project_ids %}
           AND v.project_id = ANY(%(project_ids)s)
        {% endif %}
        {% if region_ids %}
           AND v.region_id = ANY(%(region_ids)s)
        {% endif %}
        {% if statuses %}
           AND v.status::text = ANY(%(statuses)s)
        {% endif %}
    {% if limit %}
    LIMIT %(limit)s
    {% endif %}"""
    return db_query(ctx, 'vpcdb', query, project_ids=project_ids, region_ids=region_ids, statuses=statuses, limit=limit)


def get_operation(ctx, operation_id):
    query = """SELECT * FROM vpc.operations WHERE
        operation_id = %(operation_id)s"""
    operation = db_query(
        ctx,
        'vpcdb',
        query,
        operation_id=operation_id,
        fetch_single=True,
    )

    if not operation:
        raise OperationNotFound(operation_id)

    return operation


def list_operations(
    ctx,
    network_ids=None,
    project_ids=None,
    region_ids=None,
    statuses=None,
    limit=None,
):
    query = """SELECT
        *
    FROM
        vpc.operations op
    WHERE true
        {% if project_ids %}
           AND op.project_id = ANY(%(project_ids)s)
        {% endif %}
        {% if region_ids %}
           AND op.region_id = ANY(%(region_ids)s)
        {% endif %}
        {% if statuses %}
           AND op.status::text = ANY(%(statuses)s)
        {% endif %}
        {% if network_ids %}
           AND (
                op.metadata#>>'{network_id}' = ANY(%(network_ids)s)
             OR op.state#>>'{network_id}' = ANY(%(network_ids)s)
            )
        {% endif %}
    {% if limit %}
    LIMIT %(limit)s
    {% endif %}"""
    return db_query(
        ctx,
        'vpcdb',
        query,
        network_ids=network_ids,
        project_ids=project_ids,
        region_ids=region_ids,
        statuses=statuses,
        limit=limit,
    )


def wait_operation(ctx, operation_id):
    if dry_run_mode(ctx):
        print(operation_id)
        return

    task = get_operation(ctx, operation_id)
    print_response(
        ctx,
        OrderedDict(
            (
                ('operation_id', task['operation_id']),
                ('project_id', task['project_id']),
                ('action', task['action']),
                ('cloud_provider', task['cloud_provider']),
                ('region', task['region']),
                ('metadata', task['metadata']),
            )
        ),
    )

    started = False
    polling_interval = get_polling_interval(ctx)
    while True:
        if not started and task['start_time']:
            print(f'{format_timestamp(ctx, task["start_time"])} operation started')
            started = True

        if task['status'] == 'DONE':
            break

        time.sleep(polling_interval)
        task = get_operation(ctx, operation_id)

    print(f'{format_timestamp(ctx, task["finish_time"])} operation finished')
    return task

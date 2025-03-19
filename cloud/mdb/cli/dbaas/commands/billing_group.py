from click import argument, option
from click import group, pass_context

from cloud.mdb.cli.dbaas.internal.billing import (
    BillType,
    list_bills,
    disable_billing,
    enable_billing,
)

from cloud.mdb.cli.common.prompts import confirm_dangerous_action
from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.common.parameters import FieldsParamType, ListParamType
from cloud.mdb.cli.dbaas.internal.intapi import ClusterType
from cloud.mdb.cli.dbaas.internal.metadb.cluster import get_clusters
from cloud.mdb.cli.dbaas.internal.metadb.common import to_cluster_type

FIELD_FORMATTERS = {
    'cluster_type': to_cluster_type,
}


class BillFields(FieldsParamType):
    name = 'bill_fields'

    def __init__(self):
        super().__init__(
            [
                'cluster_id',
                'cluster_type',
                'bill_type',
                'from_ts',
                'until_ts',
                'updated_at',
            ]
        )


@group('billing')
def billing_group():
    """Manage billing in billingdb"""
    pass


@billing_group.command('list')
@option(
    '--bill-type',
    '--bill-types',
    'bill_types',
    type=ListParamType(BillType()),
    help='Filter objects to output by one or several bill types. Multiple values can be specified through a comma.',
)
@option(
    '--cluster-type',
    '--cluster-types',
    'cluster_types',
    type=ListParamType(ClusterType()),
    help='Filter objects to output by one or several cluster types. Multiple values can be specified through a comma.',
)
@option('-l', '--limit', type=int, default=1000, help='Limit the max number of objects in the output.')
@option(
    '--fields',
    type=BillFields(),
    default='cluster_id,cluster_type,bill_type,from_ts,until_ts,updated_at',
    help='Fields to output. The value "all" equals to all available fields.'
    ' Default is "cluster_id,cluster_type,bill_type,from_ts,until_ts,updated_at".',
)
@option('-q', '--quiet', is_flag=True, help='Output only cluster IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_bill(ctx, bill_types, cluster_types, limit, fields, quiet, separator):
    """
    List billing metadata.
    """
    result = list_bills(ctx, bill_types, cluster_types, limit)
    print_response(
        ctx,
        result,
        default_format='table',
        field_formatters=FIELD_FORMATTERS,
        fields=fields,
        quiet=quiet,
        separator=separator,
        limit=limit,
    )


@billing_group.command('enable')
@argument('cluster_ids', type=ListParamType())
@option(
    '--bill-type',
    'bill_type',
    type=BillType(),
    help='Target bill type.',
)
@pass_context
def enable_billing_command(ctx, cluster_ids, bill_type):
    """
    Enable specified billing type for clusters. Do nothing for already billed clusters.
    """
    confirm_msg = 'You are going to perform potentially dangerous action and enable billing for clusters.'
    confirm_dangerous_action(confirm_msg, False)

    clusters = get_clusters(ctx, cluster_ids=cluster_ids)
    enable_billing(
        ctx,
        [
            {
                'cid': c['id'],
                'type': c['type'],
                'bill_type': bill_type,
            }
            for c in clusters
        ],
    )


@billing_group.command('disable')
@argument('cluster_ids', type=ListParamType())
@option(
    '--bill-type',
    '--bill-types',
    'bill_types',
    type=BillType(),
    help='Target bill type.',
)
@pass_context
def disable_billing_command(ctx, cluster_ids, bill_types):
    """
    Disable specified billing types for clusters.
    """
    confirm_msg = 'You are going to perform potentially dangerous action and disable billing for clusters.'
    confirm_dangerous_action(confirm_msg, False)

    disable_billing(ctx, cluster_ids, bill_types)

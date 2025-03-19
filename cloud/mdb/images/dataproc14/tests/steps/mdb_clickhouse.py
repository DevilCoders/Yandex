"""
Steps related to managed clickhouse cluster
"""

from behave import given, then
from helpers import mdb_clickhouse as ch


@given('clickhouse cluster')
def step_clickhouse_create(ctx):
    """
    Request to create clickhouse cluster
    """
    ctx.clickhouse_cluster_name = str(ctx.id)
    ctx.clickhouse_operation_id = ch.cluster_create(ctx, ctx.clickhouse_cluster_name)
    ctx.clickhouse_cluster_id = ch.cluster_by_name(ctx, ctx.clickhouse_cluster_name).id
    ctx.clickhouse_hosts = [host.name for host in ch.cluster_hosts(ctx, ctx.clickhouse_cluster_id)]
    ctx.state['render']['clickhouse_hosts'] = ctx.clickhouse_hosts


@then('clickhouse created')
def step_clickhouse_created(ctx):
    """
    Wait till clickhouse created
    """
    ch.cluster_wait_create(ctx, ctx.clickhouse_operation_id)
    ctx.clickhouse_operation_id = None


@then('clickhouse deleted')
def step_clickhouse_deleted(ctx):
    """
    Put cluster name to ctx
    """
    ch.cluster_delete(ctx, ctx.clickhouse_cluster_id)
    ctx.clickhouse_cluster_id = None
    ctx.clickhouse_cluster_name = None
    ctx.clickhouse_hosts = None

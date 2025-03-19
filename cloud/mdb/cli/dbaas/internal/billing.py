import click

from typing import Iterable, Mapping

from cloud.mdb.cli.dbaas.internal.db import db_query

BILL_TYPES = [
    'BACKUP',
    'CH_CLOUD_STORAGE',
]


class BillType(click.Choice):
    """
    Command-line parameter type for bill type.
    """

    name = 'bill_type'

    def __init__(self):
        super().__init__(BILL_TYPES)


def list_bills(
    ctx,
    bill_types=None,
    cluster_types=None,
    limit=None,
):
    query = """SELECT
        *
    FROM
        billing.tracks t
    WHERE true
        {% if bill_types %}
           AND t.bill_type = ANY(%(bill_types)s)
        {% endif %}
        {% if cluster_type %}
           AND t.cluster_type = ANY(%(cluster_types)s)
        {% endif %}
    {% if limit %}
    LIMIT %(limit)s
    {% endif %}"""
    return db_query(ctx, 'billingdb', query, bill_types=bill_types, cluster_types=cluster_types, limit=limit)


def enable_billing(ctx, bill_info: Iterable[Mapping[str, str]]):
    query = """INSERT INTO billing.tracks
        (cluster_id, cluster_type, bill_type, from_ts, until_ts, updated_at)
    VALUES
    {{ inserted_values }}
    ON CONFLICT (cluster_id, bill_type) DO NOTHING
    """

    format_val = lambda c: f"('{c['cid']}', '{c['type']}', '{c['bill_type']}', now(), NULL, NULL)"
    return db_query(ctx, 'billingdb', query, fetch=False, inserted_values=",\n\t".join(map(format_val, bill_info)))


def disable_billing(ctx, cluster_ids, bill_types):
    query = """DELETE FROM billing.tracks
    WHERE
        bill_type = ANY(%(bill_types)s) AND
        cluster_id = ANY(%(clsuter_ids)s)"""
    return db_query(ctx, 'billingdb', query, cluster_ids=cluster_ids, bill_types=bill_types)

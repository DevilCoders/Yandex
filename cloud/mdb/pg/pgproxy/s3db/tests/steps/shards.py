# coding: utf-8

import helpers
import subprocess
from behave import then, when


@when(u'we refresh shard statistic on "{db_connstr}"')
def refresh_statistic(context, db_connstr):
    helpers.run_script(context, 's3meta', 'update_shard_stat', ['-d', db_connstr])


@when(u'we refresh shards statistics on "{n:d}" meta db')
def step_refresh_statistic(context, n):
    refresh_statistic(context, helpers.get_connstring(context, 'meta_rw', n))


@when(u'we list shards statistics')
def step_get_shard_statistic(context):
    context.result = context.connect.get("""
        SELECT
            shard_id,
            sum(buckets_count) AS buckets_count,
            sum(chunks_count) AS chunks_count
        FROM plproxy.dynamic_query_meta($$
            SELECT shard_id, buckets_count, chunks_count
            FROM s3.shard_stat
        $$) f(shard_id int, buckets_count bigint, chunks_count bigint)
        GROUP BY shard_id
        ORDER BY shard_id
    """)
    helpers.assert_no_errcode(context)
    helpers.assert_correct_number_of_rows(context, 2)


@then(u'we get following shards statistics')
def step_get_objects(context):
    helpers.assert_compare_objects_list(context)

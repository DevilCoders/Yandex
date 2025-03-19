"""Commmon steps that used in some feature files."""

from __future__ import unicode_literals

import logging

from behave import given, when, then

from buckets import clear_table
from helpers import get_connstring, update_bucket_stat, assert_no_errcode

log = logging.getLogger(__name__)


@given('empty DB')
def step_empty_db(context):
    for meta_connect in context.meta_connects:
        clear_table(meta_connect, 's3.chunks_delete_queue')
        clear_table(meta_connect, 's3.chunks')
        clear_table(meta_connect, 's3.buckets')
        clear_table(meta_connect, 's3.buckets_usage')
        clear_table(meta_connect, 's3.buckets_size')
        context.execute_steps('''
            When we refresh shard statistic on "{conn}"
        '''.format(
            conn=meta_connect.connstring,
        ))
        update_bucket_stat(context, meta_connect.connstring)

    for db_conn in context.db_connects:
        clear_table(db_conn, 's3.objects')
        clear_table(db_conn, 's3.object_parts')
        clear_table(db_conn, 's3.storage_delete_queue')
        clear_table(db_conn, 's3.billing_delete_queue')
        clear_table(db_conn, 's3.chunks')
        clear_table(db_conn, 's3.chunks_counters')
        clear_table(db_conn, 's3.chunks_counters_queue')
        clear_table(db_conn, 's3.buckets_usage')
        clear_table(db_conn, 's3.object_delete_markers')
        clear_table(db_conn, 's3.objects_noncurrent')
        clear_table(db_conn, 's3.completed_parts')
        clear_table(db_conn, 's3.inflights')


@given('refreshed buckets statistics on "{n:d}" meta db')
def refresh_buckets_statistics_(context, n):
    update_bucket_stat(context, get_connstring(context, 'meta_rw', n))


@given('refreshed chunk counters on "{n:d}" meta db')
def refresh_chunk_counters(context, n):
    context.execute_steps(
        'When we update chunks counters on "{n}" meta db'.format(n=n)
    )


@when('we refresh all statistic')
def step_refresh_statistic(context):
    context.execute_steps('''
        When we update chunk counters on "0" db
        And we update chunk counters on "1" db
        And we update chunks counters on "0" meta db
        And we update chunks counters on "1" meta db
        And we refresh shards statistics on "0" meta db
        And we refresh shards statistics on "1" meta db
    ''')

    for meta_connect in context.meta_connects:
        context.execute_steps('''
            When we refresh shard statistic on "{conn}"
        '''.format(
            conn=meta_connect.connstring,
        ))
        update_bucket_stat(context, meta_connect.connstring)


@then('we get no error')
def step_get_no_error(context):
    assert_no_errcode(context)


@then('we get an error with code "{pgcode}"')
def step_get_error(context, pgcode):
    log.info(context.result)
    assert pgcode == context.result.errcode, "We don't throw %s code." % pgcode

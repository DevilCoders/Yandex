# -*- coding: utf-8 -*-
import logging
import yaml
import time

from behave import then, when

import helpers

log = logging.getLogger('storage_delete_queue')


@when(u'we list all in the storage delete queue')
def step_list_storage_delete_queue(context):
    context.result = context.connect.get(
        """
    SELECT *
    FROM plproxy.dynamic_query_db($$
        SELECT bid, name, part_id, data_size, mds_namespace, mds_couple_id,
            mds_key_version, mds_key_uuid, remove_after_ts, deleted_ts,
            created, data_md5, parts_count, storage_class
        FROM s3.storage_delete_queue
        WHERE bid = %(bid)s
        ORDER BY remove_after_ts ASC,
            name ASC, part_id ASC
    $$) as (bid uuid, name text COLLATE "C", part_id int,
            data_size bigint, mds_namespace text COLLATE "C", mds_couple_id int,
            mds_key_version int, mds_key_uuid uuid,
            remove_after_ts timestamptz,
            deleted_ts timestamptz, created timestamptz,
            data_md5 uuid, parts_count int, storage_class int);
    """, bid=context.data['bid'])
    helpers.assert_no_errcode(context)


@then(u'we get the following deleted object(s) as a result')
def step_check_delete_object(context):
    context.result.records.sort(key=lambda x: (x['name'], x['part_id']))
    helpers.assert_compare_objects_list(context)


@when(u'we delete "{name}" in the storage delete queue with following attributes')
def step_delete(context, name):
    attrs = yaml.safe_load(context.text)

    shard = helpers.get_object_shard(context, name)

    context.result = context.db_connects[shard].get_func(
        'v1_code.remove_from_storage_delete_queue',
        i_bid=context.data['bid'],
        i_name=name,
        i_part_id=attrs['part_id'],
        i_mds_namespace=attrs['mds_namespace'],
        i_mds_couple_id=attrs['mds_couple_id'],
        i_mds_key_version=attrs['mds_key_version'],
        i_mds_key_uuid=attrs['mds_key_uuid'],
    )


@when(u'we list up to "{limit:d}" deleted for at least "{delay}" object(s) in delete queue')
def step_list_delete_queue_delay(context, limit, delay):
    context.result = context.connect.get(
        """
    SELECT *
    FROM plproxy.dynamic_query_db($$
        SELECT * FROM v1_code.list_deleted_objects(
            i_delay => %(delay)s,
            i_limit => %(limit)s
        )
        WHERE bid = %(bid)s
    $$) as (bid uuid, name text COLLATE "C", part_id int,
            data_size bigint, mds_namespace text COLLATE "C", mds_couple_id int,
            mds_key_version int, mds_key_uuid uuid,
            remove_after_ts timestamptz, storage_class int);
    """, bid=context.data['bid'], delay=delay, limit=limit)
    helpers.assert_no_errcode(context)


@when(u'we list up to "{limit:d}" deleted object(s) in delete queue')
def step_list_delete_queue(context, limit):
    step_list_delete_queue_delay(context, limit, '0 seconds')


@when(u'delay deletion of "{name}" deleted object with following attributes by "{delay}"')
def step_delay_deletion(context, name, delay):
    attrs = yaml.safe_load(context.text)

    shard = helpers.get_object_shard(context, name)
    context.result = context.db_connects[shard].get_func(
        'v1_code.delay_deletion',
        i_bid=context.data['bid'],
        i_name=name,
        i_part_id=attrs['part_id'],
        i_mds_namespace=attrs['mds_namespace'],
        i_mds_couple_id=attrs['mds_couple_id'],
        i_mds_key_version=attrs['mds_key_version'],
        i_mds_key_uuid=attrs['mds_key_uuid'],
        i_delay=delay,
    )


@when(u'we wait "{sec:f}" seconds')
def step_wait(context, sec):
    time.sleep(sec)


@then(u'bucket "{bucket_name}" in delete queue has "{objects_count:d}" object(s) of size "{objects_size:d}"')
def step_check_bucket_delete_queue_objects(context, bucket_name, objects_count, objects_size):
    context.result = context.connect.get_func(
        'v1_code.bucket_info',
        i_name=bucket_name,
    )
    helpers.assert_no_errcode(context)
    helpers.assert_correct_number_of_rows(context, 1)
    bid = context.result.records[0]['bid']

    context.result = context.connect.get(
        """
    SELECT sum(cnt) as cnt,
        sum(data_size) as data_size
    FROM plproxy.dynamic_query_db($$
        SELECT count(*), coalesce(sum(data_size)::bigint, 0)
            FROM s3.storage_delete_queue
        WHERE bid = %(bid)s AND part_id IS NULL
    $$) as (cnt bigint, data_size bigint);
    """, bid=bid)
    helpers.assert_no_errcode(context)
    helpers.assert_correct_number_of_rows(context, 1)
    res = context.result.records[0]
    helpers.assert_obj_has(res, 'cnt', objects_count)
    helpers.assert_obj_has(res, 'data_size', objects_size)


@then(u'bucket "{bucket_name}" in delete queue has "{objects_count:d}" object part(s) of size "{objects_size:d}"')
def step_check_bucket_delete_queue_object_parts(context, bucket_name, objects_count, objects_size):
    context.result = context.connect.get_func(
        'v1_code.bucket_info',
        i_name=bucket_name,
    )
    helpers.assert_no_errcode(context)
    helpers.assert_correct_number_of_rows(context, 1)
    bid = context.result.records[0]['bid']

    context.result = context.connect.get(
        """
    SELECT sum(cnt) as cnt,
        sum(data_size) as data_size
    FROM plproxy.dynamic_query_db($$
        SELECT count(*), coalesce(sum(data_size)::bigint, 0)
            FROM s3.storage_delete_queue
        WHERE bid = %(bid)s AND part_id IS NOT NULL
    $$) as (cnt bigint, data_size bigint);
    """, bid=bid)
    helpers.assert_no_errcode(context)
    helpers.assert_correct_number_of_rows(context, 1)
    res = context.result.records[0]
    helpers.assert_obj_has(res, 'cnt', objects_count)
    helpers.assert_obj_has(res, 'data_size', objects_size)


@when(u'we run copy delete queue script on "{shard:d}" db')
def step_run_copy_delete_queue_script(context, shard):
    args = [
        '--db-connstring',
        helpers.get_connstring(context, 'db_rw', shard),
    ]
    helpers.run_script(context, 's3db', 'copy_delete_queue', args)


@when(u'we list all in the billing delete queue')
def step_list_billing_delete_queue(context):
    context.result = context.connect.get(
        """
    SELECT *
    FROM plproxy.dynamic_query_db($$
        SELECT bid, name, part_id, data_size, deleted_ts,
            created, storage_class, status
        FROM s3.billing_delete_queue
        ORDER BY deleted_ts ASC, name ASC, part_id ASC
    $$) as (bid uuid, name text COLLATE "C", part_id int,
            data_size bigint, deleted_ts timestamptz, created timestamptz,
            storage_class int, status int);
    """)
    helpers.assert_no_errcode(context)


@when(u'we reset created time on storage_delete_queue "{obj_name}"')
def step_storage_delete_queue_reset_created(context, obj_name):
    for db_conn in context.db_connects:
        db_conn.query("""
        UPDATE s3.storage_delete_queue SET created = TO_TIMESTAMP(0)
        WHERE name = %(name)s
      """, name=obj_name)
    helpers.assert_no_errcode(context)

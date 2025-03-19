# coding: utf-8

from behave import given, step, then, when
import yaml

from helpers import assert_compare_objects_list
from helpers import assert_compare_one_entity_with_data
from helpers import assert_correct_number_of_rows
from helpers import assert_no_errcode
from helpers import assert_obj_has
from environment import Connection

import logging
from random import randint
from uuid import uuid4

log = logging.getLogger('buckets')


def clear_table(connect, table_name):
    res = connect.get('DELETE FROM {} RETURNING *'.format(table_name))
    if res.errcode is not None:
        log.error(res)
        raise RuntimeError('failed to clear table {}'.format(table_name))


@given(u'buckets owner account "{service_id:d}"')
def step_set_owner_account(context, service_id):
    context.service_id = service_id


@when(u'we list all buckets of account "{service_id:d}"')
def step_list_buckets(context, service_id):
    context.result = context.connect.get_func(
        'v1_code.list_buckets',
        i_service_id=service_id,
    )


@when(u'we list all buckets')
def step_list_buckets_of_owner_account(context):
    if not hasattr(context, 'service_id'):
        raise RuntimeError('buckets owner account is not set')

    step_list_buckets(context, service_id=context.service_id)


@when(u'we list all public buckets')
def step_list_public_buckets(context):
    step_list_buckets(context, service_id=None)


@then(u'we get empty result')
def step_empty_result(context):
    assert len(context.result.records) == 0, 'Result is not empty!'


@given(u'a bucket with name "{bucket_name}" of account "{service_id:d}" and max size "{max_size}"')
def step_get_bucket(context, bucket_name, service_id, max_size):
    context.bucket_name = bucket_name

    context.result = context.connect.get_func(
        'v1_code.bucket_info',
        i_name=context.bucket_name,
    )

    created = False
    if context.result.errcode is not None:
        context.result = context.connect.get_func(
            'v1_code.add_bucket',
            i_name=context.bucket_name,
            i_service_id=service_id,
            i_max_size=max_size,
        )
        created = True
    else:
        if context.result.records[0]['service_id'] != service_id:
            raise RuntimeError(
                "bucket is owned by another account: bucket='{}', service_id='{}'".format(
                    context.result.records[0], service_id))
        if context.result.records[0]['max_size'] != max_size:
            raise RuntimeError(
                "bucket have max_size set to a different value: bucket='{}', max_size='{}'".format(
                    context.result.records[0], max_size))

    bucket_meta_shard_id = context.pgmeta_connect.get("SELECT (hashtext(%(bucket_name)s) & (SELECT count(distinct "
                                                      "shard_id) - 1 FROM get_connections() where name='meta'))::int as shard", bucket_name=bucket_name)
    context.data = {
        'bid':      context.result.records[0]['bid'],
        'created':  context.result.records[0]['created'],
        'bucket_meta_shard_id': bucket_meta_shard_id.records[0]['shard'],
    }
    if created:
        context.data['versioning'] = 'disabled'


@given(u'a bucket with name "{bucket_name}" of account "{service_id:d}"')
def step_get_bucket_without_max_size(context, bucket_name, service_id):
    step_get_bucket(context, bucket_name=bucket_name, service_id=service_id, max_size=None)


@given(u'a bucket with name "{bucket_name}"')
def step_get_bucket_of_owner_account(context, bucket_name):
    if not hasattr(context, 'service_id'):
        raise RuntimeError('buckets owner account is not set')

    step_get_bucket_without_max_size(context, bucket_name=bucket_name, service_id=context.service_id)


@when(u'we add a bucket with name "{bucket_name}" of account "{service_id:d}"')
def step_add_bucket(context, bucket_name, service_id):
    context.bucket_name = bucket_name
    context.result = context.connect.get_func(
        'v1_code.add_bucket',
        i_name=context.bucket_name,
        i_service_id=service_id,
    )
    context.data = {}

    if context.result.errcode is None:
        context.data['bid'] = context.result.records[0]['bid']
        context.data['created'] = context.result.records[0]['created']


@when(u'we add a bucket with name "{bucket_name}"')
def step_add_bucket_of_owner_account(context, bucket_name):
    if not hasattr(context, 'service_id'):
        raise RuntimeError('buckets owner account is not set')

    step_add_bucket(context, bucket_name=bucket_name, service_id=context.service_id)


@when(u'we add public bucket with name "{bucket_name}"')
def step_add_public_bucket(context, bucket_name):
    step_add_bucket(context, bucket_name, service_id=None)


@when(u'we get info about bucket with name "{bucket_name}"')
def step_bucket_info_by_name(context, bucket_name):
    context.bucket_name = bucket_name
    step_bucket_info(context)


@when(u'we get bucket info')
def step_bucket_info(context):
    context.result = context.connect.get_func(
        'v1_code.bucket_info',
        i_name=context.bucket_name
    )


@then(u'we get bucket with following attributes')
def step_get_bucket_info(context):
    assert_compare_one_entity_with_data(context, ['bid', 'created'])


@then(u'we get the following buckets')
def step_validate_buckets_list(context):
    assert_compare_objects_list(context)


@when(u'we change bucket "{bucket_name}" attributes to')
def step_modify_named_bucket(context, bucket_name):
    context.bucket_name = bucket_name
    step_modify_bucket(context)


@when(u'we change bucket attributes to')
def step_modify_bucket(context):
    attrs = yaml.safe_load(context.text)
    context.result = context.connect.get_func(
        'v1_code.modify_bucket',
        i_name=context.bucket_name,
        i_versioning=attrs.get('versioning'),
        i_banned=attrs.get('banned'),
        i_max_size=attrs.get('max_size'),
        i_anonymous_read=attrs.get('anonymous_read'),
        i_anonymous_list=attrs.get('anonymous_list'),
        i_service_id=attrs.get('service_id'),
    )
    if context.result.errcode is not None:
        return
    if attrs.get('versioning'):
        context.data['versioning'] = attrs['versioning']


@when(u'we drop a bucket with name "{bucket_name}"')
def step_drop_bucket(context, bucket_name):
    context.bucket_name = bucket_name
    context.result = context.connect.get_func(
        'v1_code.drop_bucket',
        i_name=context.bucket_name,
    )


@when(u'we get chunk info for object "{object_name}"')
def step_get_chunk_info(context, object_name):
    context.object_name = object_name
    context.result = context.connect.get_func(
        'v1_impl.get_object_chunk',
        i_bucket_name=context.bucket_name,
        i_name=context.object_name,
        i_write=True,
    )


@when(u'we delete this chunk')
def delete_chunk(context):
    context.result = context.connect.get(
        """
        SELECT * FROM plproxy.dynamic_query_db($$
            DELETE FROM s3.chunks
                WHERE bid = %(bid)s AND cid = %(cid)s
            RETURNING bid, cid
        $$) AS f(
            bid uuid,
            cid bigint
        )
        """,
        bid=context.data['bid'],
        cid=context.data.get('cid', context.result.records[0]['cid']),
    )


@then(u'we get a chunk with following attributes')
def step_get_chunk(context):
    assert_compare_one_entity_with_data(context, ['bid', ])


@then(u'chunks for object "{object_name}" on meta and db are the same')
def chunks_same(context, object_name):
    step_get_chunk_info(context, object_name)
    assert_correct_number_of_rows(context, 1)
    meta_chunk = context.result.records[0]
    shard_connstring = context.connect.get_func(
        'plproxy.get_connstring',
        i_db_type='db_rw',
        i_shard_id=meta_chunk['shard_id'],
    )
    db = Connection(shard_connstring[0][0]['get_connstring'])
    context.result = db.get("""
        SELECT * FROM s3.chunks
            WHERE bid = %(bid)s AND cid = %(cid)s
    """, bid=meta_chunk['bid'], cid=meta_chunk['cid'])
    assert_correct_number_of_rows(context, 1)
    db_chunk = context.result.records[0]
    for key in ['bid', 'cid', 'start_key', 'end_key']:
        assert_obj_has(db_chunk, key, meta_chunk[key])


@when(u'we list all chunks in delete queue')
def step_list_deleted_chunks(context):
    context.result = context.connect.get(
        """
        SELECT * FROM plproxy.dynamic_query_meta($$
            SELECT start_key, end_key, shard_id
                FROM s3.chunks_delete_queue
        $$) AS f(
            start_key text COLLATE "C",
            end_key text COLLATE "C",
            shard_id int
        )
        """)
    assert_no_errcode(context)


@when(u'we add {bucket_size:d} randomly generated objects')
def step_add_bucket_with_data(context, bucket_size):
    for i in range(0, bucket_size):
        context.result = context.connect.get_func(
            'v1_code.add_object',
            i_bucket_name=context.bucket_name,
            i_bid=context.data['bid'],
            i_versioning=context.data['versioning'],
            i_name='object_%d' % i,
            i_data_size=randint(1, 1e8),
            i_data_md5=str(uuid4()),
            i_mds_namespace='namespace',
            i_mds_couple_id=randint(10, 1e3),
            i_mds_key_version=1,
            i_mds_key_uuid=str(uuid4()),
        )


@when(u'we get first object name on "{shard:d}" db')
def get_random_object(context, shard):
    conn = context.db_connects[shard]
    context.result = conn.get("SELECT name FROM s3.objects LIMIT 1")
    assert_no_errcode(context)
    context.object_name = context.result.records[0]['name']


@step(u'we enable bucket versioning')
def step_enable_bucket_versioning(context):
    context.result = context.connect.get_func(
        'v1_code.modify_bucket',
        i_name=context.bucket_name,
        i_versioning='enabled',
        i_banned=None,
        i_max_size=None,
        i_anonymous_read=None,
        i_anonymous_list=None,
        i_service_id=None,
    )
    if context.result.errcode is not None:
        return
    context.data['versioning'] = 'enabled'


@step(u'we suspend bucket versioning')
def step_suspend_bucket_versioning(context):
    context.result = context.connect.get_func(
        'v1_code.modify_bucket',
        i_name=context.bucket_name,
        i_versioning='suspended',
        i_banned=None,
        i_max_size=None,
        i_anonymous_read=None,
        i_anonymous_list=None,
        i_service_id=None,
    )
    if context.result.errcode is not None:
        return
    context.data['versioning'] = 'suspended'

# coding: utf-8
import copy
import logging

import yaml
import json
from behave import given, step, then, when

import helpers


log = logging.getLogger('disabled')


def get_db(context, object_name):
    shard = helpers.get_object_shard(context, object_name)
    return context.db_connects[shard]


@given(u'bucket "{bucket_name}" and object "{object_name}"')
def step_get_chunk_info(context, bucket_name, object_name):
    context.execute_steps(u'''
        Given a bucket with name "{bucket_name}"
    '''.format(
        bucket_name=bucket_name,
    ))
    context.data['bid'] = context.result.records[0]['bid']

    context.execute_steps(u'''
        Given a bucket with name "{bucket_name}"
        When we get chunk info for object "{object_name}"
    '''.format(
        bucket_name=bucket_name,
        object_name=object_name,
    ))
    context.data['cid'] = context.result.records[0]['cid']

    context.data['bucket_name'] = bucket_name
    context.data['object_name'] = object_name


def get_cid(context):
    context.data['cid'] = context.connect.get_func(
        'v1_impl.get_object_chunk',
        i_bucket_name=context.bucket_name,
        i_name=context.object_name,
        i_write=True,
    ).records[0]['cid']


@step(u'we add object "{object_name}" with following attributes')
def step_add_object(context, object_name):
    context.object_name = object_name
    get_cid(context)
    attrs = helpers.random_mds_key()
    attrs['data_md5'] = helpers.format_fake_uuid(0)
    attrs.update(yaml.safe_load(context.text))

    result = get_db(context, object_name).get_func(
        'v1_code.add_object',
        i_bucket_name=context.bucket_name,
        i_bid=context.data['bid'],
        i_cid=None,
        i_versioning=context.data.get('versioning', 'disabled'),
        i_name=context.object_name,
        i_data_size=attrs['data_size'],
        i_data_md5=attrs['data_md5'],
        i_mds_namespace=attrs['mds_namespace'],
        i_mds_couple_id=attrs['mds_couple_id'],
        i_mds_key_version=attrs['mds_key_version'],
        i_mds_key_uuid=attrs['mds_key_uuid'],
        i_storage_class=attrs.get('storage_class')
    )

    helpers.save_recent_version(context, result)
    context.result = result


@when(u'we get info about object with name "{object_name}"')
def step_object_version_info_by_name(context, object_name):
    context.object_name = object_name
    get_cid(context)

    context.result = get_db(context, object_name).get_func(
        'v1_code.object_info',
        i_bucket_name=context.bucket_name,
        i_bid=context.data['bid'],
        i_name=context.object_name,
    )


@then(u'we can get info about this object')
def step_object_version_info(context):
    get_cid(context)

    context.result = get_db(context, context.object_name).get_func(
        'v1_code.object_info',
        i_bucket_name=context.bucket_name,
        i_bid=context.data['bid'],
        i_name=context.object_name,
    )
    helpers.assert_no_errcode(context)


@then(u'our object has attributes')
def step_get_object_attrs(context):
    get_cid(context)
    helpers.assert_compare_one_entity_with_data(context, ['bid'])


@when(u'we list all objects in a bucket')
def step_list_all_objects(context):
    context.result = context.connect.get_func(
        'v1_code.list_objects',
        i_bucket_name=context.bucket_name,
        i_bid=context.data['bid'],
        i_prefix=None,
        i_delimiter=None,
        i_start_after=None,
    )
    helpers.assert_no_errcode(context)


@when(u'we list all objects in a bucket with prefix "{prefix}", delimiter "{delimiter}" and start_after "{start_after}"')
def step_list_objects_delimiter_start_after(context, prefix, delimiter, start_after):
    context.prefix = prefix if prefix != 'NULL' else None
    context.delimiter = delimiter if delimiter != 'NULL' else None
    context.start_after = start_after if start_after != 'NULL' else None

    context.result = context.connect.get_func(
        'v1_code.list_objects',
        i_bucket_name=context.bucket_name,
        i_bid=context.data['bid'],
        i_prefix=context.prefix,
        i_delimiter=context.delimiter,
        i_start_after=context.start_after,
    )
    helpers.assert_no_errcode(context)


@then(u'we get following objects')
def step_get_objects(context):
    helpers.assert_compare_objects_list(context)


@when(u'we drop object with name "{object_name}"')
def step_drop_object(context, object_name):
    context.object_name = object_name

    context.result = context.connect.get_func(
        'v1_code.drop_object',
        i_bucket_name=context.bucket_name,
        i_bid=context.data['bid'],
        i_versioning=context.data.get('versioning', 'disabled'),
        i_name=context.object_name,
    )


@when(u'we drop following multiple objects')
def step_drop_multiple_objects(context):
    raw_multiple_drop_objects = yaml.safe_load(context.text or '') or []
    multiple_drop_objects = [
        helpers.AS(
            'v1_code.multiple_drop_object',
            (
                raw_multiple_drop_object['name'].encode('utf8').decode('latin1'),
                raw_multiple_drop_object.get('created'),
            ),
        )
        for raw_multiple_drop_object in raw_multiple_drop_objects
    ]

    context.result = context.connect.get_func(
        'v1_code.drop_multiple_objects',
        i_bucket_name=context.bucket_name,
        i_bid=context.data['bid'],
        i_versioning=context.data.get('versioning', 'disabled'),
        i_multiple_drop_objects=multiple_drop_objects,
    )


@then(u'we got result of multiple objects delete')
def step_got_multiple_object_deletion_result(context):
    helpers.assert_no_errcode(context)
    expected = yaml.safe_load(context.text) or []
    helpers.compare_objects(expected, context.result.records, ignore_list_order=True)


@when(u'we drop following multiple recent objects')
def step_drop_multiple_recent_objects(context):
    raw_multiple_drop_objects = yaml.safe_load(context.text or '') or []
    multiple_drop_objects = [
        helpers.AS(
            'v1_code.multiple_drop_object',
            (
                raw_multiple_drop_object['name'].encode('utf8').decode('latin1'),
                context.objects[raw_multiple_drop_object['name']]['created'],
            ),
        )
        for raw_multiple_drop_object in raw_multiple_drop_objects
    ]

    context.result = context.connect.get_func(
        'v1_code.drop_multiple_objects',
        i_bucket_name=context.bucket_name,
        i_bid=context.data['bid'],
        i_versioning=context.data.get('versioning', 'disabled'),
        i_multiple_drop_objects=multiple_drop_objects,
    )


@then(u'we got result of multiple recent objects delete')
def step_got_multiple_recent_object_deletion_result(context):
    helpers.assert_no_errcode(context)
    expected = yaml.safe_load(context.text) or []

    for record in context.result.records:
        record['created'] = context.objects[record['name']]['created']

    helpers.compare_objects(expected, context.result.records, ignore_list_order=True)


def start_multipart_upload(context, attrs):
    context.result = get_db(context, context.object_name).get_func(
        'v1_code.start_multipart_upload',
        i_bucket_name=context.bucket_name,
        i_bid=context.data['bid'],
        i_name=context.object_name,
        i_mds_namespace=attrs.get('mds_namespace'),
        i_mds_couple_id=attrs.get('mds_couple_id'),
        i_mds_key_version=attrs.get('mds_key_version'),
        i_mds_key_uuid=attrs.get('mds_key_uuid'),
        i_storage_class=attrs.get('storage_class'),
        i_metadata=attrs.get('metadata'),
    )
    context.objects[context.object_name] = context.result.records[0]
    helpers.assert_no_errcode(context)


@when(u'we start multipart upload of object "{object_name}"')
def step_start_multipart(context, object_name):
    context.object_name = object_name
    get_cid(context)

    attrs = yaml.safe_load(context.text or '') or {}
    start_multipart_upload(context, attrs)


@then(u'we get the following object part')
def step_get_object_part(context):
    helpers.assert_compare_one_entity_with_data(context, ['bid'])


@then(u'we get following object parts')
def step_get_object_parts(context):
    helpers.assert_compare_objects_list(context)


@then(u'we get the following multipart info')
def step_get_multipart_object_parts(context):
    helpers.assert_compare_objects_list(context)


@given(u'a multipart upload in bucket "{bucket_name}" for object "{object_name}"')
def step_given_multipart_upload(context, bucket_name, object_name):
    context.bucket_name = bucket_name
    context.object_name = object_name

    context.execute_steps(u'''
        Given a bucket with name "{bucket_name}"
        When we get chunk info for object "{object_name}"
    '''.format(
        bucket_name=bucket_name,
        object_name=object_name,
    ))
    context.data['cid'] = context.result.records[0]['cid']
    context.data['bucket_name'] = bucket_name
    context.data['object_name'] = object_name

    context.result = context.connect.get_func(
        'v1_code.list_multipart_uploads',
        i_bucket_name=context.bucket_name,
        i_bid=context.data['bid'],
        i_prefix=context.object_name,
    )

    if len(context.result.records) != 0:
        context.objects[object_name] = context.result.records[0]
    else:
        context.execute_steps(u'''
            Given a bucket with name "{bucket_name}"
            When we start multipart upload of object "{object_name}"
        '''.format(
            bucket_name=bucket_name,
            object_name=object_name,
        ))


def upload_part(context, object_name, obj, attrs):
    meta = None
    if 'encryption' in attrs:
        meta = json.dumps({ 'encryption': { 'meta': attrs['encryption'] } })
    context.result = get_db(context, object_name).get_func(
        'v1_code.upload_object_part',
        i_bucket_name=context.bucket_name,
        i_bid=context.data['bid'],
        i_name=object_name,
        i_object_created=obj.get('created'),
        i_part_id=attrs['part_id'],
        i_data_size=attrs['data_size'],
        i_data_md5=attrs['data_md5'],
        i_mds_namespace=attrs['mds_namespace'],
        i_mds_couple_id=attrs['mds_couple_id'],
        i_mds_key_version=attrs['mds_key_version'],
        i_mds_key_uuid=attrs['mds_key_uuid'],
        i_metadata=meta,
    )


@when(u'we upload part for object "{object_name}"')
def step_upload_part(context, object_name):
    step_attrs = yaml.safe_load(context.text)
    attrs = helpers.random_mds_key()
    attrs['data_md5'] = helpers.format_fake_uuid(step_attrs['part_id'])
    attrs.update(step_attrs)

    obj = context.objects.get(object_name, {})
    upload_part(context, object_name, obj, attrs)


@when(u'we add multipart object "{object_name}" with following part sizes')
def step_upload_parts(context, object_name):
    attrs = yaml.safe_load(context.text)
    context.object_name = object_name
    start_multipart_upload(context, {})
    obj = context.objects[object_name]
    parts_data = []
    for i, size in enumerate(attrs):
        part_attrs = helpers.random_mds_key()
        part_attrs['part_id'] = i + 1
        part_attrs['data_size'] = size
        part_attrs['data_md5'] = helpers.format_fake_uuid(i)
        upload_part(context, object_name, obj, part_attrs)
        parts_data.append(part_attrs)
    complete_attrs = {
        'data_md5': helpers.format_fake_uuid(0),
        'parts_data': parts_data,
    }
    complete_multipart(context, object_name, obj, complete_attrs)


@when(u'we abort multipart upload for object "{object_name}"')
def step_abort_multipart(context, object_name):
    obj = context.objects.get(object_name, {})
    context.result = get_db(context, object_name).get_func(
        'v1_code.abort_multipart_upload',
        i_bucket_name=context.bucket_name,
        i_bid=context.data['bid'],
        i_name=object_name,
        i_created=obj.get('created'),
    )


@when(u'we list current parts for object "{object_name}"')
def step_list_current_partitions(context, object_name):
    obj = context.objects.get(object_name, {})
    context.result = context.connect.get_func(
        'v1_code.list_current_parts',
        i_bucket_name=context.bucket_name,
        i_bid=context.data['bid'],
        i_name=object_name,
        i_created=obj.get('created'),
    )
    helpers.assert_no_errcode(context)

@when(u'we list object parts for object "{object_name}"')
def step_list_object_partitions(context, object_name):
    obj = context.objects.get(object_name, {})
    context.result = get_db(context, object_name).get("""
        SELECT
            %(i_name)s AS name, part_id, part_data_size data_size, part_data_md5 data_md5,
            NULL mds_namespace, part_mds_couple_id mds_couple_id,
            part_mds_key_version mds_key_version, part_mds_key_uuid mds_key_uuid
        FROM v1_code.object_info_with_parts(
            i_bucket_name => %(i_bucket_name)s,
            i_bid => %(i_bid)s,
            i_name => %(i_name)s,
            i_created => NULL,
            i_null_version => false,
            i_range_start => 0,
            i_range_end => 0
        )
        WHERE part_data_size IS NOT NULL""",
        i_bucket_name=context.bucket_name,
        i_bid=context.data['bid'],
        i_name=object_name,
        i_created=None,
        i_null_version=False,
        i_range_start=0,
        i_range_end=-1,
    )
    helpers.assert_no_errcode(context)

def complete_multipart(context, object_name, obj, attrs):
    context.object_name = object_name
    context.result = get_db(context, object_name).get_func(
        'v1_code.complete_multipart_upload',
        i_bucket_name=context.bucket_name,
        i_bid=context.data['bid'],
        i_versioning=context.data.get('versioning', 'disabled'),
        i_name=object_name,
        i_created=obj.get('created'),
        i_data_md5=attrs['data_md5'],
        i_parts_data=[helpers.AS('v1_code.object_part_data', (i['part_id'], i['data_md5']))
                      for i in attrs['parts_data']],
    )
    if context.result.errcode is None:
        helpers.save_recent_version(context, context.result)


@step(u'we complete the following multipart upload')
def step_complete_multipart(context):
    attrs = yaml.safe_load(context.text)
    object_name = attrs['name']
    obj = context.objects.get(object_name, {})
    complete_multipart(context, object_name, obj, attrs)


@when(u'we list multipart uploads with following arguments')
def step_list_multipart_uploads(context):
    attrs = yaml.safe_load(context.text)

    context.result = context.connect.get_func(
        'v1_code.list_multipart_uploads',
        i_bucket_name=context.bucket_name,
        i_bid=context.data['bid'],
        i_prefix=attrs['prefix'],
        i_delimiter=attrs['delimiter'],
        i_start_after_key=attrs['start_after_key'],
        i_start_after_created=attrs['start_after_created'],
    )
    helpers.assert_no_errcode(context)


@when(u'we update object "{object_name}" metadata with following info')
def step_update_object_metadata(context, object_name):
    attrs = yaml.safe_load(context.text) or {}

    context.result = context.connect.get_func(
        'v1_code.update_object_metadata',
        i_bucket_name=context.bucket_name,
        i_bid=context.data['bid'],
        i_name=object_name,
        i_mds_namespace=attrs.get('mds_namespace'),
        i_mds_couple_id=attrs.get('mds_couple_id'),
        i_mds_key_version=attrs.get('mds_key_version'),
        i_mds_key_uuid=attrs.get('mds_key_uuid'),
        i_storage_class=attrs.get('storage_class')
    )


@then(u'we get following object')
def step_get_object(context):
    helpers.assert_no_errcode(context)

    # Don't compare parts because parts are now stored separately
    records = context.result.records
    helpers.assert_correct_number_of_rows(context, 1)
    expect_attrs = yaml.safe_load(context.text) or {}
    del expect_attrs['parts']
    helpers.compare_objects(expect_attrs, records[0])


@step(u'we add multipart object "{object_name}" with following attributes')
def step_add_multipart_object(context, object_name):
    context.object_name = object_name
    get_cid(context)
    attrs = yaml.safe_load(context.text)

    start_multipart_upload(context, attrs)
    helpers.assert_no_errcode(context)

    parts = attrs.pop('parts')
    assert parts, 'parts required'

    obj = context.objects.get(object_name, {})

    for part in parts:
        upload_part(context, object_name, obj, part)
        helpers.assert_no_errcode(context)

    complete_multipart_attrs = copy.deepcopy(attrs)
    parts_data = [{'part_id': part['part_id'], 'data_md5': part['data_md5']} for part in parts]
    complete_multipart_attrs['parts_data'] = parts_data
    complete_multipart(context, object_name, obj, complete_multipart_attrs)
    helpers.assert_no_errcode(context)


@when(u'we recover object with name "{name}"')
def step_recovery_object(context, name):
    shard = helpers.get_object_shard(context, name)
    context.result = context.db_connects[shard].get_func(
        'v1_code.recovery_object',
        i_bid=context.data['bid'],
        i_name=name,
    )
    created = context.db_connects[shard].get(
        "SELECT * FROM s3.objects WHERE bid=%(bid)s AND name=%(name)s",
        bid=context.data['bid'],
        name=name,
    )
    if context.result.errcode is None:
        context.objects[name] = created.records[0]


@when(u'we abort following multipart uploads')
def step_abort_multiparts(context):
    raw_uploads = yaml.safe_load(context.text or '') or []

    uploads = []
    for u in raw_uploads:
        obj = context.objects.get(u, {})
        uploads.append(helpers.AS(
            'v1_code.lifecycle_element_key',
            (
                u.encode('utf8').decode('latin1'),
                obj.get('created', '1970-01-01 00:00:00+00'),
            ),
        ))
    if not uploads:
        return
    log.info("abort uploads %s", [str(x.obj) for x in uploads])
    shard = helpers.get_object_shard(context, raw_uploads[0])
    context.result = context.db_connects[shard].get_func(
        'v1_code.abort_multiple_uploads',
        i_bid=context.data['bid'],
        i_uploads=uploads,
    )


@when(u'we expire following objects')
def step_expire_objects(context):
    raw_objects = yaml.safe_load(context.text or '') or []

    objects = []
    for o in raw_objects:
        obj = context.objects.get(o)
        objects.append(helpers.AS(
            'v1_code.lifecycle_element_key',
            (
                o.encode('utf8').decode('latin1'),
                obj['created'] if obj else '1970-01-01 00:00:00+00',
            ),
        ))
    if not objects:
        return
    log.info("expire objects %s", [str(x.obj) for x in objects])
    shard = helpers.get_object_shard(context, raw_objects[0])
    context.result = context.db_connects[shard].get_func(
        'v1_code.expire_multiple_objects',
        i_bid=context.data['bid'],
        i_versioning=context.data.get('versioning', 'disabled'),
        i_objects=objects,
    )


@when(u'we transfer following objects to storage class "{storage_class:d}"')
def step_transfer_objects(context, storage_class):
    raw_objects = yaml.safe_load(context.text or '') or []

    objects = []
    for o in raw_objects:
        obj = context.objects.get(o)
        objects.append(helpers.AS(
            'v1_code.lifecycle_element_key',
            (
                o.encode('utf8').decode('latin1'),
                obj['created'] if obj else '1970-01-01 00:00:00+00',
            ),
        ))
    if not objects:
        return
    log.info("transfer objects %s", [str(x.obj) for x in objects])
    shard = helpers.get_object_shard(context, raw_objects[0])
    context.result = context.db_connects[shard].get_func(
        'v1_code.transfer_multiple_objects',
        i_bid=context.data['bid'],
        i_storage_class=storage_class,
        i_objects=objects,
    )


@when(u'we expire following noncurrent versions')
def step_expire_versions(context):
    raw_objects = yaml.safe_load(context.text or '') or []

    objects = []
    for o in raw_objects:
        versions = context.data['recent_versions'].get(o, ())
        obj = None
        if len(versions) > 0:
            obj = versions[0]
        objects.append(helpers.AS(
            'v1_code.lifecycle_element_key',
            (
                o.encode('utf8').decode('latin1'),
                obj['created'] if obj else '1970-01-01 00:00:00+00',
            ),
        ))
    if not objects:
        return
    log.info("expire objects %s", [str(x.obj) for x in objects])
    shard = helpers.get_object_shard(context, raw_objects[0])
    context.result = context.db_connects[shard].get_func(
        'v1_code.expire_multiple_noncurrent',
        i_bid=context.data['bid'],
        i_objects=objects,
    )


@when(u'we transfer following noncurrent versions to storage class "{storage_class:d}"')
def step_transfer_versions(context, storage_class):
    raw_objects = yaml.safe_load(context.text or '') or []

    objects = []
    for o in raw_objects:
        versions = context.data['recent_versions'].get(o, ())
        obj = None
        if len(versions) > 0:
            obj = versions[0]
        objects.append(helpers.AS(
            'v1_code.lifecycle_element_key',
            (
                o.encode('utf8').decode('latin1'),
                obj['created'] if obj else '1970-01-01 00:00:00+00',
            ),
        ))
    if not objects:
        return
    log.info("transfer objects %s", [str(x.obj) for x in objects])
    shard = helpers.get_object_shard(context, raw_objects[0])
    context.result = context.db_connects[shard].get_func(
        'v1_code.transfer_multiple_noncurrent',
        i_bid=context.data['bid'],
        i_storage_class=storage_class,
        i_objects=objects,
    )


@when(u'we cleanup following delete markers')
def step_cleanup_delete_markers(context):
    raw_objects = yaml.safe_load(context.text or '') or []

    objects = []
    for o in raw_objects:
        obj = context.objects.get(o)
        objects.append(helpers.AS(
            'v1_code.lifecycle_element_key',
            (
                o.encode('utf8').decode('latin1'),
                obj['created'] if obj else '1970-01-01 00:00:00+00',
            ),
        ))
    if not objects:
        return
    log.info("cleanup objects %s", [str(x.obj) for x in objects])
    shard = helpers.get_object_shard(context, raw_objects[0])
    context.result = context.db_connects[shard].get_func(
        'v1_code.cleanup_delete_markers',
        i_bid=context.data['bid'],
        i_objects=objects,
    )


@then(u'we get following lifecycle results')
def step_get_lifecycle_results(context):
    helpers.assert_compare_objects_unordered(context)

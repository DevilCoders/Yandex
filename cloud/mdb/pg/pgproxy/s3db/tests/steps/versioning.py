# coding: utf-8

import logging

import yaml
from behave import step, then, when

import helpers


log = logging.getLogger('versioning')


@step(u'we add delete marker with name "{object_name}"')
def step_add_delete_marker(context, object_name):
    context.object_name = object_name
    shard = helpers.get_object_shard(context, context.object_name)
    context.result = context.db_connects[shard].get_func(
        'v1_code.add_delete_marker',
        i_bid=context.data['bid'],
        i_name=context.object_name,
        i_versioning=context.data.get('versioning', 'disabled'),
        i_created=None,
        i_creator_id=None,
    )
    helpers.save_recent_version(context, context.result)


@step(u'we remove last version of object "{object_name}"')
def step_remove_last_version(context, object_name):
    last_version = context.data['recent_versions'][context.object_name].pop()
    version_param = last_version['created']
    if last_version['null_version']:
        version_param = None

    shard = helpers.get_object_shard(context, object_name)
    context.result = context.db_connects[shard].get_func(
        'v1_code.drop_object_version',
        i_bid=context.data['bid'],
        i_name=object_name,
        i_created=version_param,
    )
    helpers.assert_no_errcode(context)
    context.objects[object_name] = last_version


@when(u'we list all versions in a bucket with prefix "{prefix}", delimiter "{delimiter}" and start_after "{start_after}"')
def step_list_objects_delimiter_start_after(context, prefix, delimiter, start_after):
    context.prefix = prefix if prefix != 'NULL' else None
    context.delimiter = delimiter if delimiter != 'NULL' else '\x01'
    context.start_after = start_after if start_after != 'NULL' else None

    shard = helpers.get_object_shard(context, "")
    context.result = context.db_connects[shard].get_func(
        'v1_code.list_object_version_names',
        i_bid=context.data['bid'],
        i_prefix=context.prefix,
        i_delimiter=context.delimiter,
        i_start_after=context.start_after,
        i_start_after_created=None,
        i_limit=1000,
    )
    helpers.assert_no_errcode(context)


@when(u'we list all versions in a bucket')
def step_list_all_objects(context):
    shard = helpers.get_object_shard(context, "")
    context.result = context.db_connects[shard].get_func(
        'v1_code.list_object_version_names',
        i_bid=context.data['bid'],
        i_prefix=None,
        i_delimiter='\\',
        i_start_after=None,
        i_start_after_created=None,
        i_limit=1000,
    )
    helpers.assert_no_errcode(context)


@when(u'we list all noncurrent versions in a bucket')
def step_list_all_noncurrent(context):
    shard = helpers.get_object_shard(context, "")
    context.result = context.db_connects[shard].get("""
        SELECT * FROM s3.objects_noncurrent WHERE bid=%(bid)s
    """, bid=context.data['bid'])
    helpers.assert_no_errcode(context)


@then(u'we get following list items')
def step_got_listing(context):
    helpers.assert_compare_objects_list(context)


@when(u'we get info about previous version of object "{object_name}"')
def step_object_version_info_by_name(context, object_name):
    last_version = context.data['recent_versions'][context.object_name][-2]
    version_param = last_version['created']
    if last_version['null_version']:
        version_param = None

    shard = helpers.get_object_shard(context, object_name)
    context.result = context.db_connects[shard].get_func(
        'v1_code.object_version_info',
        i_bid=context.data['bid'],
        i_name=object_name,
        i_created=version_param,
    )
    helpers.assert_no_errcode(context)


@when(u'we drop following multiple object\'s last versions')
def step_drop_multiple_object_versions(context):
    raw_multiple_drop_objects = yaml.safe_load(context.text or '') or []
    rv = context.data['recent_versions']
    last_versions = []
    for name in raw_multiple_drop_objects:
        el = {'name': name, 'created': '1970-01-01 00:00:00'}
        if name in rv:
            lv = rv[name].pop()
            el['created'] = None if lv['null_version'] else lv['created']
        last_versions.append(el)

    multiple_drop_objects = [
        helpers.AS(
            'v1_code.multiple_drop_object',
            (
                v['name'].encode('utf8').decode('latin1'),
                v['created'],
            ),
        )
        for v in last_versions
    ]

    shard = helpers.get_object_shard(context, "")
    context.result = context.db_connects[shard].get_func(
        'v1_code.drop_multiple_versions',
        i_bid=context.data['bid'],
        i_versioning=context.data.get('versioning', 'disabled'),
        i_multiple_drop_objects=multiple_drop_objects,
    )

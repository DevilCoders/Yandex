import json
import logging
import time
import yaml

from datetime import datetime
from behave import given, step, then, when

import helpers


log = logging.getLogger('disabled')


def assert_mds_key(expected_inflight, db_inflight):
    assert expected_inflight.get('mds_couple_id') == db_inflight['mds_couple_id']
    assert expected_inflight.get('mds_key_version') == db_inflight['mds_key_version']
    assert expected_inflight.get('mds_key_uuid') == db_inflight['mds_key_uuid']


def assert_inflight_key(object_name, db_inflight):
    assert db_inflight['bid'] == db_inflight['bid']
    assert db_inflight['name'] == object_name
    assert db_inflight['object_created'] == db_inflight['object_created']
    assert db_inflight['inflight_created'] == db_inflight['inflight_created']


def assert_json_metadata(raw_metadata, db_metadata):
    expected_metadata = json.loads(raw_metadata or '{}')
    json_db_metadata = db_metadata or {}

    assert expected_metadata == json_db_metadata, \
        'Mismatch in metadata. Got "{db_metadata}", expected "{expected_metadata}"'.format(
            db_metadata=json_db_metadata,
            expected_metadata=expected_metadata,
        )


def get_db(context, object_name):
    shard = helpers.get_object_shard(context, object_name)
    return context.db_connects[shard]


@when(u'we start inflight for object "{object_name}"')
def step_start_inflight(context, object_name):
    attrs = helpers.random_mds_key()
    attrs.update(yaml.safe_load(context.text))

    raw_metadata = attrs.get('metadata', '{}')

    metadata = json.loads(raw_metadata)

    obj = context.objects.setdefault(object_name, {
        'created': context.data.get('created', datetime.now()),
    })

    context.result = get_db(context, object_name).get_func(
        'v1_code.start_inflight_upload',
        i_bid=context.data['bid'],
        i_object_name=object_name,
        i_object_created=obj.get('created'),
        i_mds_couple_id=attrs.get('mds_couple_id'),
        i_mds_key_version=attrs.get('mds_key_version'),
        i_mds_key_uuid=attrs.get('mds_key_uuid'),
        i_metadata=raw_metadata,
    )

    helpers.assert_no_errcode(context)
    context.data['inflight_created'] = context.result.records[0]['inflight_created']


@then(u'inflight has part "{part_number:d}" for object "{object_name}"')
def step_has_inflight(context, part_number, object_name):
    if context.result == None:
        RuntimeError("inflight wasn't initialized")

    obj = context.objects.get(object_name, {})

    context.result = get_db(context, object_name).get(
        """
        SELECT bid,
            name,
            object_created,
            inflight_created,
            part_id,
            mds_couple_id,
            mds_key_version,
            mds_key_uuid,
            metadata
        FROM s3.inflights WHERE
                bid = '{bid}' AND
                name = '{object_name}' AND
                part_id = {part_id} AND
                object_created = '{object_created}' AND
                inflight_created = '{inflight_created}'
        """.format(
            bid=context.data['bid'],
            object_name=object_name,
            part_id=part_number,
            object_created=obj.get('created'),
            inflight_created=context.data['inflight_created'],
        )
    )

    helpers.assert_no_errcode(context)
    helpers.assert_correct_number_of_rows(context, 1)

    expected = helpers.yaml.safe_load(context.text) or {}
    assert context.result.records[0]['part_id'] == part_number, \
        "Wrong part number {raw_part}, expected {part_number}".format(
            raw_part=context.result.records[0]['part_id'],
            part_number=part_number,
        )

    assert_json_metadata(
        raw_metadata=expected.get('metadata'),
        db_metadata=context.result.records[0]['metadata'],
    )

    assert_mds_key(
        expected_inflight=expected,
        db_inflight=context.result.records[0],
    )

    assert_inflight_key(
        object_name=object_name,
        db_inflight=context.result.records[0],
    )


@when(u'we complete inflight upload for object "{object_name}"')
def step_complete_inflight_upload(context, object_name):
    attrs = {}
    if context.text:
        attrs.update(yaml.safe_load(context.text))

    obj = context.objects.get(object_name, {})

    current_timestamp = datetime.now()
    context.result = get_db(context, object_name).get_func(
        'v1_code.complete_inflight_uploads',
        i_bid=context.data['bid'],
        i_object_name=object_name,
        i_object_created=obj.get('created'),
        i_inflight_created=context.data.get('inflight_created', current_timestamp),
        i_object_migrated=attrs.get('migrated'),
    )


@when(u'we failed to complete not started inflight upload for object "{object_name}"')
def step_complete_inflight_upload(context, object_name):
    context.execute_steps(u'''
        When we complete inflight upload for object "{object_name}"
    '''.format(
        object_name=object_name,
    ))

    context.execute_steps(u'Then there is `no such upload` error')


@then(u'there is `no such upload` error')
def step_no_such_inflight(context):
    assert context.result.errcode == 'S3M01', \
        "Should be `no such upload` error"


@when(u'we complete inflight upload for object "{object_name}" w/o errors')
def step_complete_inflight_upload(context, object_name):
    context.execute_steps(u'''
        When we complete inflight upload for object "{object_name}":
        """
        {text}
        """
    '''.format(
        object_name=object_name,
        text=context.text or '',
    ))

    assert context.result.errcode is None, \
        "Inflight operation should complete without errors and with non-empty result set, but got error {err_code}".format(
           err_code=context.result.errcode,
        )


@then(u'inflight operation completed with error or returned empty result')
def step_inflight_completed_with_errors(context):
    assert context.result.errcode is not None or len(context.result.records) == 0, \
        "Inflight operation should fail with error"


@then(u'inflight has parts count of "{count:d}" for object "{object_name}"')
def step_inflight_parts_count(context, count, object_name):

    obj = context.objects.get(object_name, {
        'created': context.data.get('created')
    })

    current_timestamp = datetime.now()
    context.result = get_db(context, object_name).get(
        """
        SELECT count(*) as cnt
        FROM s3.inflights WHERE
                bid = '{bid}'
                AND name = '{object_name}'
                AND object_created = '{object_created}'
                AND inflight_created = '{inflight_created}'
                AND part_id > 0
        """.format(
            bid=context.data['bid'],
            object_name=object_name,
            object_created=obj.get('created'),
            inflight_created=context.data.get('inflight_created', current_timestamp),
        )
    )

    helpers.assert_no_errcode(context)
    helpers.assert_correct_number_of_rows(context, 1)

    assert context.result.records[0]['cnt'] == count, \
        'Count mismatch, should be {count} but got {cnt}'.format(
            count=count,
            cnt=context.result.records[0]['cnt'],
        )


@then(u'there are no inflights for object {object_name}')
def step_is_empty(context, object_name):
    step_inflight_parts_count(context, 0, object_name)


@when(u'we add inflight for object "{object_name}" with part_id "{part_id:d}"')
def step_add_inflight(context, object_name, part_id):
    attrs = helpers.random_mds_key()
    attrs.update(yaml.safe_load(context.text))

    obj = context.objects.setdefault(object_name, {
        'created': context.data['created'],
    })

    context.result = get_db(context, object_name).get_func(
        'v1_code.add_inflight_upload',
        i_bid=context.data['bid'],
        i_object_name=object_name,
        i_object_created=obj.get('created'),
        i_inflight_created=context.data.get('inflight_created'),
        i_part_id=part_id,
        i_mds_couple_id=attrs.get('mds_couple_id'),
        i_mds_key_version=attrs.get('mds_key_version'),
        i_mds_key_uuid=attrs.get('mds_key_uuid'),
        i_metadata=attrs.get('metadata'),
    )


@when(u'we abort inflight for object "{object_name}"')
def step_abort_inflight(context, object_name):
    current_timestamp = datetime.now()

    context.result = get_db(context, object_name).get_func(
        'v1_code.abort_inflight_uploads',
        i_bid=context.data['bid'],
        i_object_name=object_name,
        i_object_created=context.data.get('created'),
        i_inflight_created=context.data.get('inflight_created', current_timestamp),
    )

    helpers.assert_no_errcode(context)


@then(u'object "{object_name}" has fields')
def step_object_with_fields(context, object_name):
    attrs = helpers.random_mds_key()
    attrs.update(yaml.safe_load(context.text))

    context.result = get_db(context, object_name).get(
        """
        SELECT
            parts_count,
            mds_couple_id,
            mds_key_version,
            mds_key_uuid,
            metadata,
            array_to_json(parts)
        FROM v1_code.object_info(
            i_bucket_name => '{bucket}',
            i_bid => '{bid}',
            i_name => '{object_name}'
        )""".format(
            bucket='inflights',
            bid=context.data['bid'],
            object_name=object_name,
        )
    )

    helpers.assert_no_errcode(context)
    helpers.assert_correct_number_of_rows(context, 1)

    result = context.result.records[0]
    parts_result = context.result.records[0].get('array_to_json', [])

    assert attrs['parts_count'] == len(parts_result or [])
    assert attrs['parts_count'] == (result['parts_count'] or 0)

    assert attrs['mds_couple_id'] == result['mds_couple_id']
    assert attrs['mds_key_version'] == result['mds_key_version']
    assert attrs['mds_key_uuid'] == result['mds_key_uuid']

    for i, part in enumerate(attrs.get('parts_data') or []):
        assert part['part_id'] == parts_result[i]['part_id']
        assert part['mds_couple_id'] == parts_result[i]['mds_couple_id']
        assert part['mds_key_version'] == parts_result[i]['mds_key_version']
        assert part['mds_key_uuid'] == parts_result[i]['mds_key_uuid']

    raw_metadata = attrs.get('metadata')
    if raw_metadata is None:
        return

    expected_metadata = json.loads(raw_metadata)
    actual_metadata = context.result.records[0].get('metadata')

    assert expected_metadata == actual_metadata, \
        "Metadata has not expected values: expected={expected} != actual={actual}".format(
            expected=expected_metadata,
            actual=actual_metadata,
        )

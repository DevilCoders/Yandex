# coding: utf-8
import copy
import datetime
import logging
import random
import os
import subprocess
import uuid

import tenacity

try:
    import collections.abc as collections_abc
except ImportError:
    import collections as collections_abc

import psycopg2.extensions as PGE
import yaml
from behave import then, when

log = logging.getLogger(__name__)


class UTC(datetime.tzinfo):
    """UTC"""

    def utcoffset(self, dt):
        return datetime.timedelta(0)

    def tzname(self, dt):
        return "UTC"

    def dst(self, dt):
        return datetime.timedelta(0)


def run_script(context, db, script, args, assert_on_error=True):
    script = get_script_path(context, db, script)
    process = subprocess.Popen([script] + args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    exitcode = process.wait()
    out, err = process.communicate()
    context.exitcode = exitcode
    if assert_on_error:
        assert exitcode == 0, '{script} failed with error: {code}, stdout: {out}, stderr: {err}'.format(
            code=exitcode, out=out, err=err, script=script)
    return out.strip()


def update_bucket_stat(context, db_connstr):
    pgmeta_connstr = str(context.CONNSTRING['pgmeta'])
    return run_script(context, 's3meta', 'update_bucket_stat', ['-d', db_connstr, '-p', pgmeta_connstr])


@then('we fail')
def step_we_fail(context):
    raise AssertionError('You asked - we failed.')


def get_connstring(context, db_type, shard_id):
    db_connstring = context.connect.get_func(
        'plproxy.get_connstring',
        i_db_type=db_type,
        i_shard_id=shard_id,
    )
    return db_connstring.records[0]['get_connstring']


def get_bucket_bid(context, bucket_name):
    result = context.connect.get("""
        SELECT bid FROM v1_code.bucket_info(
            %(bucket_name)s
        )
    """, bucket_name=bucket_name)
    return result.records[0]['bid']


def get_object_shard(context, name):
    context.result = context.connect.get_func(
        'v1_impl.get_object_shard',
        i_bucket_name=context.bucket_name,
        i_name=name,
        i_write=True,
    )
    assert_no_errcode(context)
    assert_correct_number_of_rows(context, 1)
    return context.result.records[0]['get_object_shard']


@then(u'we have no prepared transactions')
def step_no_prepared_xacts(context):
    context.result = context.connect.get(
        """
        SELECT * FROM plproxy.dynamic_query_db('
            SELECT * FROM pg_prepared_xacts
        ') AS f(
            transaction xid,
            gid text,
            prepared timestamptz,
            owner name,
            database name
        )
        UNION ALL
        SELECT * FROM plproxy.dynamic_query_meta('
            SELECT * FROM pg_prepared_xacts
        ') AS f(
            transaction xid,
            gid text,
            prepared timestamptz,
            owner name,
            database name
        )
        """
    )
    assert_no_errcode(context)
    assert_correct_number_of_rows(context, 0)


@then(u'we have no transactions with application_name like s3_script_%')
def step_no_scripts_transactions(context):
    context.result = context.connect.get(
        """
        SELECT * FROM plproxy.dynamic_query_db('
            select application_name from pg_stat_activity
        ') AS f(
            application_name text
        )
        UNION ALL
        SELECT * FROM plproxy.dynamic_query_meta('
            select application_name from pg_stat_activity
        ') AS f(
            application_name text
        )
        """
    )
    assert_no_errcode(context)
    assert not any([r['application_name'].startswith('s3_script_') for r in context.result.records])


class AS(object):
    def __init__(self, type_name, obj):
        self.type_name = type_name
        self.obj = obj

    def __conform__(self, proto):
        if proto is PGE.ISQLQuote:
            return self

    def getquoted(self):
        return PGE.adapt(self.obj).getquoted() + '::' + self.type_name


def assert_no_errcode(context):
    assert context.result.errcode is None, \
        'Query has not completed successfully. Errcode: %s. Errmsg %s' \
        % (context.result.errcode, context.result.errmsg)


def assert_correct_number_of_rows(context, expected):
    assert len(context.result.records) == expected, \
        'Expected %d object(s) but got the following: %r' % (expected, context.result)
    return context.result.records


def assert_obj_has(obj, attr, value):
    assert attr in obj, 'Attribute %r not in object' % attr
    assert obj[attr] == value, 'Expected %r for %s but got %r' % (value, attr, obj.get(attr))


def assert_obj_has_not_equal(obj, attr, value):
    assert attr in obj and obj[attr] != value, \
        'Expected not %r for %s but got %r' % (value, attr, obj.get(attr))


def assert_compare_one_entity_with_data(context, keys='*'):
    """Compare entities by `data` and `text`.

    In case when keys not passed, use keys from context.text
    """
    entity = assert_correct_number_of_rows(context, 1)[0]

    # yaml.safe_load("") = None, but we need dict
    expect_attrs = yaml.safe_load(context.text) or {}

    if keys == '*':
        keys = expect_attrs.keys()

    for key in keys:
        assert_obj_has(entity, key, context.data[key])

    for k, v in expect_attrs.iteritems():
        assert_obj_has(entity, k, v)


def assert_compare_one_entity_with_result(context):
    """Compare entities by `result` and `text`.

    In case when keys not passed, use keys from context.text.
    """
    records = context.result.records
    assert_correct_number_of_rows(context, 1)

    # yaml.safe_load("") = None, but we need dict
    expect_attrs = yaml.safe_load(context.text) or {}

    compare_objects(expect_attrs, records[0])


def assert_not_equal_one_entity(context, keys):
    entity = assert_correct_number_of_rows(context, 1)[0]
    for key in keys:
        assert_obj_has_not_equal(entity, key, context.data[key])


def assert_compare_objects_list(context):
    """
        context.text  - expected results
        context.result - actual results
    """
    # yaml.safe_load("") = None, but we expect some list
    expected = yaml.safe_load(context.text) or []
    assert_correct_number_of_rows(context, len(expected))

    log.info(expected)
    for i in xrange(0, len(expected)):
        expect_attrs = expected[i]
        for k, v in expect_attrs.iteritems():
            assert_obj_has(context.result.records[i], k, v)


def assert_compare_objects_unordered(context):
    """
        Analogical to assert_compare_objects_list, but support unordered match.
        Restrictions: you should ensure, that expected data doesn't match some results.

        context.text  - expected results
        context.result - actual results
    """
    # yaml.safe_load("") = None, but we expect some list
    expected = yaml.safe_load(context.text) or []
    compare_objects(expected, context.result.records, ignore_list_order=True)


def get_script_path(context, db, script):
    if db:
        path = 'scripts/{db}/{script}/{script}'.format(db=db, script=script)
    else:
        path = 'scripts/{script}/{script}'.format(script=script)
    try:
        import yatest.common
        script_path = yatest.common.binary_path('cloud/mdb/pg/pgproxy/s3db/{path}'.format(path=path))
    except ImportError:
        script_path = os.path.realpath(
            os.path.join(
                os.path.dirname(context.feature.filename),
                '../../{path}.py'.format(path=path)
            )
        )
    assert os.path.isfile(script_path), (
        'Failed to look up {script}: {path}'.format(script=script, path=script_path))
    return script_path


def _compare_sequences(expected_object, result_object, ignore_list_order, including, context):
    expected_length = len(expected_object)
    result_length = len(result_object)

    if not including:
        assert expected_length == result_length, (
            'sequences have different length. Expected - %d: %s, result - %d: %s. Context: %s' % (
                expected_length,
                expected_object,
                result_length,
                result_object,
                context,
            )
        )
    if not ignore_list_order:
        for i in range(len(expected_object)):
            list_item_context = '%s[%d]' % (context, i)
            _compare_objects(
                expected_object[i],
                result_object[i],
                ignore_list_order,
                list_item_context,
                including,
            )
        return

    result_copy = copy.copy(result_object)
    for index, expected_element in enumerate(expected_object):
        list_item_context = '%s[%d]' % (context, index)
        match_count = 0
        matching_index = None
        for i in range(len(result_copy)):
            try:
                _compare_objects(
                    expected_element,
                    result_copy[i],
                    ignore_list_order,
                    list_item_context,
                    including,
                )
            except AssertionError:
                pass
            else:
                matching_index = i
                match_count += 1

        assert match_count, (
            'no matches for expected element %r. Context: %s'
            % (expected_element, list_item_context)
        )
        assert match_count == 1, (
            'some matches according to one of expected result %d, require another test data or this'
            ' checker extending. Context: %s' % (match_count, context)
        )
        result_copy.pop(matching_index)


def _compare_objects(expected_object, result_object, ignore_list_order, including, context='root'):
    log.debug('Compare objects. Expected: %r, result: %r', expected_object, result_object)
    if isinstance(expected_object, collections_abc.Mapping):
        for key in expected_object:
            assert key in result_object, "key '%s' absent in result object. Context: %s" \
                                         % (key, context)
            key_context = "%s['%s']" % (context, key)
            _compare_objects(
                expected_object[key],
                result_object[key],
                ignore_list_order,
                including,
                key_context,
            )
    elif (isinstance(expected_object, collections_abc.Sequence)
            and not isinstance(expected_object, basestring)):
        _compare_sequences(expected_object, result_object, ignore_list_order, including, context)
    else:

        if (isinstance(expected_object, basestring)
                and isinstance(result_object, datetime.datetime)):
            expected_object = datetime.datetime.strptime(expected_object, '%Y-%m-%d %H:%M:%S').replace(tzinfo=UTC())

        assert expected_object == result_object, (
            'different object values. Expected: %r, result: %r. Context: %s'
            % (expected_object, result_object, context)
        )


def compare_objects(expected_object, result_object, ignore_list_order=False, including=False):
    """Compare expected_object with result_object with skipping keys in nested structures."""
    return _compare_objects(expected_object, result_object, ignore_list_order, including)


def save_recent_version(context, result):
    if result.errcode is not None:
        return
    context.objects[context.object_name] = result.records[0]

    if 'recent_versions' not in context.data:
        context.data['recent_versions'] = {}

    context.data['recent_versions'].setdefault(context.object_name, [])
    versioning = context.data.get('versioning', 'disabled')
    if versioning == 'enabled':
        context.data['recent_versions'][context.object_name].append(result.records[0])
    elif versioning == 'suspended':
        context.data['recent_versions'][context.object_name] = [
            x for x in context.data['recent_versions'][context.object_name]
            if not x.get('null_version', True)
        ]
        context.data['recent_versions'][context.object_name].append(result.records[0])
    else:
        context.data['recent_versions'][context.object_name] = [result.records[0]]


def random_mds_key():
    num = random.randrange(9999)
    return {
        'mds_namespace': 'ns-{}'.format(num),
        'mds_couple_id': num,
        'mds_key_version': 2,
        'mds_key_uuid': str(uuid.uuid4()),
    }


def format_fake_uuid(num):
    return '-'.join(('{0}'*8, '{0}'*4, '{0}'*4, '{0}'*4, '{0}'*12)).format(str(num)[0])


def get_work_path():
    try:
        import yatest.common
        return yatest.common.ram_drive_path() or yatest.common.work_path()
    except ImportError:
        return '/tmp'


@when(u'we run s3_closer on {replica_type} with command "{command}"')
def step_run_s3_closer_script(context, replica_type, command):
    args = [
        '--db-connstring',
        context.RO_CONNSTRING['s3db01r'] if replica_type == 'replica' else context.CONNSTRING['s3db01'],
        '--pgmeta-connstring',
        str(context.CONNSTRING['pgmeta']),
        '--closer-filepath',
        os.path.join(get_work_path(), 's3_closer_' + replica_type),
    ] + command.split()
    run_script(context, '', 's3_closer', args, assert_on_error=False)


@then(u's3_closer check output on {replica_type} contains')
def step_run_s3_closer_script_check(context, replica_type):
    args = [
        '--db-connstring',
        context.RO_CONNSTRING['s3db01r'] if replica_type == 'replica' else context.CONNSTRING['s3db01'],
        '--pgmeta-connstring',
        str(context.CONNSTRING['pgmeta']),
        '--closer-filepath',
        os.path.join(get_work_path(), 's3_closer_' + replica_type),
        'check',
    ]
    result = run_script(context, '', 's3_closer', args)
    expected = yaml.safe_load(context.text) or ''
    assert expected in result


def pgcheck_closed_status(connection, expect_closed=True):
    @tenacity.retry(
        retry=tenacity.retry_if_result(lambda result: not result or result[0]['closed'] != expect_closed),
        wait=tenacity.wait_exponential(multiplier=1, max=5),
        stop=tenacity.stop_after_attempt(5),
    )
    def impl():
        result = connection.get("SELECT current_setting('pgcheck.closed', true)::bool AS closed").records
        return result
    return impl()[0]['closed']


@then('one replica is closed')
def step_one_replica_is_closed(context):
    assert pgcheck_closed_status(context.ro_connection, expect_closed=True) is True


@then('all hosts are opened')
def step_all_hosts_are_opened(context):
    for conn in context.db_connects + [context.ro_connection]:
        assert pgcheck_closed_status(conn, expect_closed=False) is False

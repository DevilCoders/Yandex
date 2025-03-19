# coding: utf-8

import os
from behave import then, when
from disabled import get_cid

import helpers


@when(u'we update chunk counters on "{n:d}" db')
def step_update_chunk_counters(context, n):
    args = [
        '--db-connstring',
        helpers.get_connstring(context, 'db_rw', n),
    ]
    helpers.run_script(context, 's3db', 'update_chunks_counters', args)


@when(u'we corrupt chunks counters for current bucket')
def step_corrupt_chunk_counters(context):
    context.result = context.connect.get(
        """
        SELECT * FROM plproxy.dynamic_query_db($$
            UPDATE s3.chunks_counters SET
                simple_objects_count = (random()*100)::bigint,
                simple_objects_size = (random()*100)::bigint,
                multipart_objects_count = (random()*100)::bigint,
                multipart_objects_size = (random()*100)::bigint,
                objects_parts_count = (random()*100)::bigint,
                objects_parts_size = (random()*100)::bigint
            WHERE bid = '{0}'
            RETURNING 1
        $$) as (cnt int);
        """.format(context.data['bid']))
    helpers.assert_no_errcode(context)


@when(u'we run repair script on "{shard:d}" db')
def step_run_repair_script(context, shard):
    args = [
        '--db-connstring',
        helpers.get_connstring(context, 'db_rw', shard),
        '--repair',
    ]
    helpers.run_script(context, 's3db', 'check_chunks_counters', args)


@then(u'we have no errors in counters')
def step_check_counters_by_script(context):
    errors_file = 'critical_errors.log'
    if os.path.exists(errors_file):
        os.remove(errors_file)
    for dbname, connstring in context.CONNSTRING.iteritems():
        if dbname.startswith('s3db'):
            args_common = [
                '--db-connstring',
                connstring,
                '--critical-errors-filepath',
                errors_file,
            ]
            helpers.run_script(context, 's3db', 'check_chunks_counters', args_common)
            helpers.run_script(context, 's3db', 'check_chunks_counters', args_common + ['--check-only-presence'])
            helpers.run_script(context, 's3db', 'check_chunks_counters', args_common + ['--storage-class', '1'])
        elif dbname.startswith('s3meta'):
            args_common = [
                '--db-connstring',
                connstring,
                '--critical-errors-filepath',
                errors_file,
                '--pgmeta-connstring',
                str(context.CONNSTRING['pgmeta']),
                '--period',
                'infinity',
                '--skip-equal-buckets-usage',
            ]
            helpers.run_script(context, 's3meta', 'check_chunks_bounds', args_common)

    if os.path.exists(errors_file) and os.stat(errors_file).st_size != 0:
        raise AssertionError('Errors file contains rows: \n%s' % '\n'.join(open(errors_file, 'r').readlines()))


@when(u'we list all chunks counters in the bucket "{bucket_name}"')
def step_list_all_chunks(context, bucket_name):
    context.result = context.connect.get_func(
        'v1_code.get_chunks_counters',
        i_bucket_name=bucket_name,
        i_bid=context.data['bid']
    )
    helpers.assert_no_errcode(context)


@then(u'bucket "{bucket_name}" has overall "{objects_count:d}" object(s) of size "{objects_size:d}"')
def step_check_overall_bucket_counters(context, bucket_name, objects_count, objects_size):
    context.result = context.connect.get(
        """
            SELECT
                sum(simple_objects_count + multipart_objects_count) AS objects_count,
                sum(simple_objects_size + multipart_objects_size) AS objects_size
            FROM
                v1_code.get_chunks_counters('{bucket_name}', '{bid}')
            GROUP BY bid
        """.format(
            bucket_name=bucket_name,
            bid=context.data['bid'],
        )
    )
    helpers.assert_no_errcode(context)
    if not context.result.records:
        res = {'objects_count': 0, 'objects_size': 0}
    else:
        res = context.result.records[0]
    helpers.assert_obj_has(res, 'objects_count', objects_count)
    helpers.assert_obj_has(res, 'objects_size', objects_size)


@then(u'for type "{object_type}" bucket "{bucket_name}" has "{objects_count:d}" object(s) of size "{objects_size:d}"')
def step_check_bucket_counters(context, bucket_name, objects_count, objects_size, object_type):
    context.result = context.connect.get(
        """
            SELECT
                sum({object_type}_count) AS objects_count,
                sum({object_type}_size) AS objects_size
            FROM
                v1_code.get_chunks_counters('{bucket_name}', '{bid}')
            GROUP BY bid
        """.format(
            bucket_name=bucket_name,
            bid=context.data['bid'],
            object_type=object_type
        )
    )
    helpers.assert_no_errcode(context)
    if not context.result.records:
        res = {'objects_count': 0, 'objects_size': 0}
    else:
        res = context.result.records[0]
    helpers.assert_obj_has(res, 'objects_count', objects_count)
    helpers.assert_obj_has(res, 'objects_size', objects_size)


@then(u'bucket "{bucket_name}" has "{objects_count:d}" object(s) of size "{objects_size:d}"')
def step_check_simple_bucket_counters(context, bucket_name, objects_count, objects_size):
    context.execute_steps(u'''
        Then for type "{object_type}" bucket "{bucket_name}" has "{objects_count}" object(s) of size "{objects_size}"
    '''.format(
        object_type="simple_objects",
        bucket_name=bucket_name,
        objects_count=objects_count,
        objects_size=objects_size
    ))


@then(u'bucket "{bucket_name}" has "{objects_count:d}" multipart object(s) of size "{objects_size:d}"')
def step_check_multipart_bucket_counters(context, bucket_name, objects_count, objects_size):
    context.execute_steps(u'''
        Then for type "{object_type}" bucket "{bucket_name}" has "{objects_count}" object(s) of size "{objects_size}"
    '''.format(
        object_type="multipart_objects",
        bucket_name=bucket_name,
        objects_count=objects_count,
        objects_size=objects_size
    ))


@then(u'bucket "{bucket_name}" has "{objects_count:d}" object part(s) of size "{objects_size:d}"')
def step_check_parts_bucket_counters(context, bucket_name, objects_count, objects_size):
    context.execute_steps(u'''
        Then for type "{object_type}" bucket "{bucket_name}" has "{objects_count}" object(s) of size "{objects_size}"
    '''.format(
        object_type="objects_parts",
        bucket_name=bucket_name,
        objects_count=objects_count,
        objects_size=objects_size
    ))


@then(u'bucket "{bucket_name}" consists of "{chunks_count:d}" chunks and has "{objects_count:d}" object(s) of size "{objects_size:d}"')
def step_check_bucket_chunk_counters(context, bucket_name, chunks_count, objects_count, objects_size):
    context.result = context.connect.get(
        """
            SELECT
                count(cid) AS chunks_count,
                sum(simple_objects_count) AS objects_count,
                sum(simple_objects_size) AS objects_size
            FROM
                v1_code.get_chunks_counters('{0}', '{1}')
            GROUP BY bid
        """.format(bucket_name, context.data['bid'])
    )
    helpers.assert_no_errcode(context)
    if not context.result.records:
        res = {'objects_count': 0, 'objects_size': 0}
    else:
        res = context.result.records[0]
    helpers.assert_obj_has(res, 'chunks_count', chunks_count)
    helpers.assert_obj_has(res, 'objects_count', objects_count)
    helpers.assert_obj_has(res, 'objects_size', objects_size)


@then(u'bucket "{bucket_name}" has "{objects_count:d}" object(s) in chunks counters queue')
def step_check_bucket_counters_queue(context, bucket_name, objects_count):
    context.result = context.connect.get(
        """
        SELECT sum(cnt) AS cnt FROM plproxy.dynamic_query_db($$
            SELECT count(*) FROM s3.chunks_counters_queue WHERE bid = '{0}'
        $$) AS (cnt bigint)
        """.format(context.data['bid'])
    )
    helpers.assert_no_errcode(context)
    helpers.assert_correct_number_of_rows(context, 1)
    res = context.result.records[0]
    helpers.assert_obj_has(res, 'cnt', objects_count)


@then(u'bucket "{bucket_name}" has "{objects_count:d}" deleted objects of size "{objects_size:d}" in counters queue')
def step_deleted_in_counters_queue(context, bucket_name, objects_count, objects_size):
    context.result = context.connect.get(
        """
        SELECT sum(deleted_objects_count) AS deleted_objects_count, sum(deleted_objects_size) as deleted_objects_size
          FROM plproxy.dynamic_query_db($$
            SELECT
              coalesce(sum(deleted_objects_count_change), 0)::bigint as deleted_objects_count,
              coalesce(sum(deleted_objects_size_change), 0)::bigint as deleted_objects_size
            FROM s3.chunks_counters_queue
            WHERE bid = '{0}'
        $$) AS (deleted_objects_count bigint, deleted_objects_size bigint)
        """.format(context.data['bid'])
    )
    helpers.assert_no_errcode(context)
    helpers.assert_correct_number_of_rows(context, 1)
    res = context.result.records[0]
    helpers.assert_obj_has(res, 'deleted_objects_count', objects_count)
    helpers.assert_obj_has(res, 'deleted_objects_size', objects_size)


@then(u'bucket "{bucket_name}" has "{objects_count:d}" active multipart updload(s) in counters queue')
def step_active_multipart_upload_in_counters_queue(context, bucket_name, objects_count):
    context.result = context.connect.get(
        """
        SELECT sum(active_multipart_count) AS active_multipart_count
          FROM plproxy.dynamic_query_db($$
            SELECT
              coalesce(sum(active_multipart_count_change), 0)::bigint as active_multipart_count
            FROM s3.chunks_counters_queue
            WHERE bid = '{0}'
        $$) AS (active_multipart_count bigint)
        """.format(context.data['bid'])
    )
    helpers.assert_no_errcode(context)
    helpers.assert_correct_number_of_rows(context, 1)
    res = context.result.records[0]
    helpers.assert_obj_has(res, 'active_multipart_count', objects_count)


@when(u'we split chunks bigger than "{threshold}" object(s) on "{n:d}" db')
def step_split_chunks(context, threshold, n):
    args = [
        '--bid',
        str(context.data['bid']),
        '--bloat-threshold',
        str(threshold),
        '--db-connstring',
        helpers.get_connstring(context, 'db_rw', n),
        '--pgmeta-connstring',
        str(context.CONNSTRING['pgmeta']),
    ]
    helpers.run_script(context, 's3db', 'chunk_splitter', args)


@when(u'we split chunk by key "{key}"')
def step_split_chunk_key(context, key):
    if 'cid' not in context.data:
        get_cid(context)
    # TODO: get_chunk_shard to separate function
    shard = context.connect.get_func(
        'v1_impl.get_chunk_shard',
        i_bucket_name=context.bucket_name,
        i_bid=context.data['bid'],
        i_cid=context.data['cid'],
    )
    db_connstr = helpers.get_connstring(context, 'db_rw',
                                        shard.records[0]['get_chunk_shard'])
    args = [
        '--bid',
        str(context.data['bid']),
        '--cid',
        str(context.data['cid']),
        '--key',
        str(key),
        '--db-connstring',
        db_connstr,
        '--pgmeta-connstring',
        str(context.CONNSTRING['pgmeta']),
    ]
    helpers.run_script(context, 's3db', 'chunk_splitter', args)


@when(u'we fail before all commits while split chunks bigger than "{threshold:d}" object(s) on "{n:d}" db')
def step_split_chunks_emulate_failure_before(context, threshold, n):
    context.execute_steps(u'''
        When we fail on step "0" while split chunks bigger than "{threshold:d}" object(s) on "{n:d}" db
    '''.format(
        threshold=threshold,
        n=n,
    ))


@when(u'we fail after "{fail_step:d}" transaction commits while split chunks bigger than "{threshold:d}" object(s) on "{n:d}" db')
def step_split_chunks_emulate_failure_after_steps(context, fail_step, threshold, n):
    context.execute_steps(u'''
        When we fail on step "{fail_step:d}" while split chunks bigger than "{threshold:d}" object(s) on "{n:d}" db
    '''.format(
        threshold=threshold,
        n=n,
        fail_step=fail_step,
    ))


@when(u'we fail on step "{fail_step:d}" while split chunks bigger than "{threshold:d}" object(s) on "{n:d}" db')
def step_split_chunks_emulate_failure(context, threshold, n, fail_step):
    args = [
        '--bid',
        str(context.data['bid']),
        '--bloat-threshold',
        str(threshold),
        '--db-connstring',
        helpers.get_connstring(context, 'db_rw', n),
        '--pgmeta-connstring',
        str(context.CONNSTRING['pgmeta']),
        '--fail-step',
        str(fail_step),
        '--sleep-interval',
        '0',
        '--attempts',
        '1',
    ]
    helpers.run_script(context, 's3db', 'chunk_splitter', args, assert_on_error=False)
    assert context.exitcode != 0, 'Split chunks exit with error: {0}'.format(context.exitcode)


@when(u'we fail on step "{fail_step:d}" while split chunks bigger than "{threshold:d}" object(s) on "{n:d}" db without attempts')
def step_split_chunks_emulate_failure_without_attempts(context, threshold, n, fail_step):
    args = [
        '--bid',
        str(context.data['bid']),
        '--bloat-threshold',
        str(threshold),
        '--db-connstring',
        helpers.get_connstring(context, 'db_rw', n),
        '--pgmeta-connstring',
        str(context.CONNSTRING['pgmeta']),
        '--fail-step',
        str(fail_step),
        '--sleep-interval',
        '0',
        '--attempts',
        '0',
    ]
    helpers.run_script(context, 's3db', 'chunk_splitter', args, assert_on_error=False)
    assert context.exitcode != 0, 'Split chunks exit with error: {0}'.format(context.exitcode)


@when(u'we run {script:w} on "{n:d}" meta db')
def step_run_script_meta(context, script, n):
    params = {
        'chunk_mover': [
            '--min-objects',
            '1',
            '--diff-threshold',
            '2',
            '--delay',
            '-1year',
        ],
        'finish_prepared_xacts': [
            '--delay',
            '-1year',
        ],
    }
    args = [
        '--db-connstring',
        helpers.get_connstring(context, 'meta_rw', n),
        '--pgmeta-connstring',
        str(context.CONNSTRING['pgmeta']),
    ]
    helpers.run_script(context, 's3meta', script, args + params.get(script, []))


@when(u'we run {script:w} on "{n:d}" meta db with fail on "{fail_step:d}" step')
def step_run_script_meta_emulate_failure(context, script, n, fail_step):
    params = {
        'chunk_mover': [
            '--min-objects',
            '1',
            '--diff-threshold',
            '2',
            '--delay',
            '-1year',
        ],
        'finish_prepared_xacts': [
            '--delay',
            '-1year',
        ],
    }
    args = [
        '--db-connstring',
        helpers.get_connstring(context, 'meta_rw', n),
        '--pgmeta-connstring',
        str(context.CONNSTRING['pgmeta']),
        '--fail-step',
        str(fail_step),
        '--attempts',
        '0',
    ] + params.get(script, [])
    helpers.run_script(context, 's3meta', script, args, assert_on_error=False)


@when(u'we run chunk_mover with zero copy timeout on "{n:d}" meta db')
def step_move_chunk_with_zero_copy_timeout(context, n):
    args = [
        '--db-connstring',
        helpers.get_connstring(context, 'meta_rw', n),
        '--pgmeta-connstring',
        str(context.CONNSTRING['pgmeta']),
        '--min-objects',
        '1',
        '--diff-threshold',
        '2',
        '--delay',
        '-1year',
        '--copy-timeout',
        '0'
    ]
    helpers.run_script(context, 's3meta', 'chunk_mover', args, assert_on_error=False)


@when(u'we run chunk_mover "{n:d}" meta db with allowed fail')
def step_run_chunk_mover_allowed_fail(context, n):
    args = [
        '--db-connstring',
        helpers.get_connstring(context, 'meta_rw', n),
        '--pgmeta-connstring',
        str(context.CONNSTRING['pgmeta']),
        '--min-objects',
        '1',
        '--diff-threshold',
        '2',
        '--delay',
        '-1year',
    ]
    helpers.run_script(context, 's3meta', 'chunk_mover', args, assert_on_error=False)


@then(u'we exit with code "{expect_exitcode:d}"')
def exit_with_code(context, expect_exitcode):
    assert context.exitcode == expect_exitcode, 'Exit with code {got} instead of {expect}'.format(
        got=context.exitcode, expect=expect_exitcode)


@when(u'we fail after "{fail_step:d}" transaction commits while move chunk on "{n:d}" meta db')
def step_move_chunk_emulate_failure_before(context, fail_step, n):
    args = [
        '--db-connstring',
        helpers.get_connstring(context, 'meta_rw', n),
        '--pgmeta-connstring',
        str(context.CONNSTRING['pgmeta']),
        '--fail-step',
        str(fail_step),
        '--attempts',
        '0',
        '--min-objects',
        '1',
        '--diff-threshold',
        '2',
        '--delay',
        '-1year',
    ]
    helpers.run_script(context, 's3meta', 'chunk_mover', args, assert_on_error=False)
    assert context.exitcode != 0, 'Chunk mover exit with error: {0}'.format(context.exitcode)


@when(u'we run merge script on "{shard:d}" db')
def step_run_merge_script(context, shard):
    args = [
        '--db-connstring',
        helpers.get_connstring(context, 'db_rw', shard),
        '--pgmeta-connstring',
        str(context.CONNSTRING['pgmeta']),
    ]
    helpers.run_script(context, 's3db', 'merge_chunks', args)


def fetch_chunks_counters_bucket_db(context, bid):
    context.result = context.connect.get("""
        SELECT bid, cid,
                simple_objects_count,
                simple_objects_size,
                multipart_objects_count,
                multipart_objects_size,
                objects_parts_count,
                objects_parts_size,
                updated_ts
          FROM plproxy.dynamic_query_db(
            $$SELECT bid, cid,
                simple_objects_count,
                simple_objects_size,
                multipart_objects_count,
                multipart_objects_size,
                objects_parts_count,
                objects_parts_size,
                updated_ts
            FROM s3.chunks_counters WHERE bid = %(bid)s$$
        ) AS (bid uuid, cid bigint,
            simple_objects_count bigint,
            simple_objects_size bigint,
            multipart_objects_count bigint,
            multipart_objects_size bigint,
            objects_parts_count bigint,
            objects_parts_size bigint,
            updated_ts timestamptz)
        ORDER BY cid
    """, bid=bid)
    helpers.assert_no_errcode(context)


def fetch_chunks_counters_bucket_meta(context, bid):
    context.result = context.connect.get("""
        SELECT bid, cid,
                simple_objects_count,
                simple_objects_size,
                multipart_objects_count,
                multipart_objects_size,
                objects_parts_count,
                objects_parts_size,
                updated_ts
          FROM plproxy.dynamic_query_meta(
            $$SELECT bid, cid,
                simple_objects_count,
                simple_objects_size,
                multipart_objects_count,
                multipart_objects_size,
                objects_parts_count,
                objects_parts_size,
                updated_ts
            FROM s3.chunks_counters WHERE bid = %(bid)s$$
        ) AS (bid uuid, cid bigint,
            simple_objects_count bigint, simple_objects_size bigint,
            multipart_objects_count bigint, multipart_objects_size bigint,
            objects_parts_count bigint, objects_parts_size bigint,
            updated_ts timestamptz)
        ORDER BY cid
    """, bid=bid)
    helpers.assert_no_errcode(context)


def fetch_chunks_bounds_bucket_db(context, bid):
    context.result = context.connect.get("""
        SELECT bid, cid, start_key, end_key
          FROM plproxy.dynamic_query_db(
            $$SELECT bid, cid, start_key, end_key
            FROM s3.chunks WHERE bid = %(bid)s$$
        ) AS (bid uuid, cid bigint, start_key text, end_key text)
        ORDER BY cid
    """, bid=bid)
    helpers.assert_no_errcode(context)


def fetch_chunks_bounds_bucket_meta(context, bid):
    context.result = context.connect.get("""
        SELECT bid, cid, start_key, end_key
          FROM plproxy.dynamic_query_meta(
            $$SELECT bid, cid, start_key, end_key
            FROM s3.chunks WHERE bid = %(bid)s$$
        ) AS (bid uuid, cid bigint, start_key text, end_key text)
        ORDER BY cid
    """, bid=bid)
    helpers.assert_no_errcode(context)


@then(u'chunks counters for bucket "{bucket_name}" on meta and db are the same')
def step_chunks_counters_bucket_same(context, bucket_name):
    bid = helpers.get_bucket_bid(context, bucket_name)
    fetch_chunks_counters_bucket_db(context, bid)
    db_chunks = context.result.records
    expected = len(db_chunks)

    fetch_chunks_counters_bucket_meta(context, bid)
    helpers.assert_correct_number_of_rows(context, expected)

    for i in xrange(0, expected):
        expect_attrs = db_chunks[i]
        for k, v in expect_attrs.iteritems():
            helpers.assert_obj_has(context.result.records[i], k, v)


@then(u'chunks counters for bucket "{bucket_name}" contains rows')
def step_chunks_counters_bucket_contains(context, bucket_name):
    bid = helpers.get_bucket_bid(context, bucket_name)
    fetch_chunks_counters_bucket_db(context, bid)
    result = context.result.records
    expected = helpers.yaml.safe_load(context.text) or []
    helpers.compare_objects(expected, result, ignore_list_order=True)


@then(u'chunks bounds for bucket "{bucket_name}" on meta and db are the same')
def step_chunks_bounds_bucket_same(context, bucket_name):
    bid = helpers.get_bucket_bid(context, bucket_name)
    fetch_chunks_bounds_bucket_db(context, bid)
    db_chunks = context.result.records
    expected = len(db_chunks)

    fetch_chunks_bounds_bucket_meta(context, bid)
    helpers.assert_correct_number_of_rows(context, expected)

    for i in xrange(0, expected):
        expect_attrs = db_chunks[i]
        for k, v in expect_attrs.iteritems():
            helpers.assert_obj_has(context.result.records[i], k, v)


@when(u'we update chunks counters on "{n:d}" meta db')
def step_update_chunk_counters_meta(context, n):
    args = [
        '--db-connstring',
        helpers.get_connstring(context, 'meta_rw', n),
        '--pgmeta-connstring',
        str(context.CONNSTRING['pgmeta']),
    ]
    helpers.run_script(context, 's3meta', 'update_chunks_counters', args)
    helpers.run_script(context, 's3meta', 'update_buckets_usage', args)


@when(u'we list all chunks in the bucket "{bucket_name}" on meta db')
def step_get_bucket_chunks_on_meta(context, bucket_name):
    context.result = context.connect.get(
        """
    SELECT * FROM plproxy.dynamic_query_meta($$
        SELECT c.* FROM s3.chunks c
            JOIN s3.buckets b USING (bid)
            WHERE b.name = %(bucket_name)s
    $$) AS (bid uuid, cid bigint, created timestamptz,
            read_only boolean, start_key text,
            end_key text, shard_id int)
    ORDER BY start_key NULLS FIRST;
    """, bucket_name=bucket_name)
    helpers.assert_no_errcode(context)


@when(u'we list all chunks on db')
def step_get_bucket_chunks_on_db(context):
    context.result = context.connect.get(
        """
        SELECT * FROM plproxy.dynamic_query_db($$
            SELECT bid, cid, start_key, end_key
            FROM s3.chunks
        $$) AS (bid uuid, cid bigint,
                start_key text, end_key text)
        ORDER BY start_key NULLS FIRST;
        """)
    helpers.assert_no_errcode(context)


@then(u'we get following chunks')
def step_check_chunks(context):
    helpers.assert_compare_objects_list(context)


@when(u'add to chunks moving queue one record on "{n:d}" meta db')
def step_add_chunk_to_move(context, n):
    conn = context.meta_connects[n]
    chunk_to_move = conn.get_func(
        'v1_code.get_chunk_to_move',
        i_min_objects_diff=1,
        i_min_objects=1,
        i_max_objects=1000,
        i_created_delay='0 min',
    )
    if not chunk_to_move.records:
        return
    chunk_moving_task = chunk_to_move.records[0]
    context.result = conn.get_func(
        'v1_code.chunk_move_queue_push',
        i_source_shard=chunk_moving_task['source_shard'],
        i_dest_shard=chunk_moving_task['dest_shard'],
        i_bid=chunk_moving_task['bid'],
        i_cid=chunk_moving_task['cid'],
    )


@when(u'we push to move queue this chunk from "{src:n}" to "{dst:n}" shard on "{n:d}" meta db')
def push_to_chunks_moving_queue(context, src, dst, n):
    cid = context.data.get('cid', context.result.records[0]['cid'])
    conn = context.meta_connects[n]
    context.result = conn.get_func(
        'v1_code.chunk_move_queue_push',
        i_source_shard=src,
        i_dest_shard=dst,
        i_bid=context.data['bid'],
        i_cid=cid,
    )


@when(u'we push to move queue all chunks on "{n:d}" meta db')
def push_to_chunks_moving_queue(context, n):
    conn = context.meta_connects[n]
    context.result = conn.get(
        """
        SELECT v1_code.chunk_move_queue_push(
            shard_id,
            (NOT shard_id::bool)::int, -- 1->0; 0->1
            bid, cid)
          FROM s3.chunks
        """
    )


@then(u'we get objects count by shard on "{n:d}" meta db')
def get_objects_count_by_shard(context, n):
    conn = context.meta_connects[n]
    context.result = conn.get(
        """
        SELECT simple_objects_count + multipart_objects_count AS c
        FROM s3.shard_stat
        ORDER BY shard_id
    """)
    helpers.assert_no_errcode(context)
    helpers.assert_compare_objects_list(context)


@when(u'we move all chunks from queue on "{n:d}" meta db')
def step_move_chunk(context, n):
    args = [
        '--db-connstring',
        helpers.get_connstring(context, 'meta_rw', n),
        '--pgmeta-connstring',
        str(context.CONNSTRING['pgmeta']),
        '--only-queue',
        '--allow-same-shard',
        '--max-threads',
        '2',
    ]
    helpers.run_script(context, 's3meta', 'chunk_mover', args)


@then(u'there are "{count:d}" object(s) in chunks moving queues')
def step_check_moving_queue_size(context, count):
    result = 0
    for conn in context.meta_connects:
        result += conn.get("SELECT count(*) FROM s3.chunks_move_queue").records[0]['count']
    assert result == count, 'chunks_move_queue has {result} rows, excepted {count}'.format(result=result, count=count)


@then(u'there are "{count:d}" object(s) on "{n:d}" db')
def step_check_objects_count(context, count, n):
    conn = context.db_connects[n]
    context.result = conn.get("SELECT count(*) FROM s3.objects")
    helpers.assert_no_errcode(context)
    result = context.result.records[0]['count']
    assert result == count, 's3.objects has {result} rows, expected {count}'.format(result=result, count=count)


@then(u'joined chunks and counters contain rows')
def step_db_joined_chunks_counters_rows(context):
    result = []
    for db_conn in context.db_connects:
        db_usage = db_conn.get("""
            SELECT start_key, end_key, storage_class,
              cc.simple_objects_count, cc.simple_objects_size,
              cc.multipart_objects_count, cc.multipart_objects_size,
              cc.objects_parts_count, cc.objects_parts_size,
              cc.active_multipart_count
            FROM s3.chunks LEFT JOIN s3.chunks_counters cc USING (bid, cid)
        """)
        for record in db_usage.records:
            result.append(record)
    expected = helpers.yaml.load(context.text) or []
    helpers.compare_objects(expected, result, ignore_list_order=True, including=True)

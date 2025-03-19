# coding: utf-8

from behave import then, when

import helpers


@when(u'we insert into chunk counters queue row with size "{size:d}" at "{ts}"')
def step_insert_chunk_counters(context, size, ts):
    meta_conn = context.meta_connects[context.data['bucket_meta_shard_id']]
    meta_data = meta_conn.get("SELECT cid, shard_id FROM s3.chunks WHERE bid=%(bid)s", bid=context.data['bid'])
    context.data['cid'] = meta_data.records[0]['cid']
    context.data['db_shard_id'] = meta_data.records[0]['shard_id']

    context.result = context.db_connects[context.data['db_shard_id']].query("""
        INSERT INTO s3.chunks_counters_queue (bid, cid, simple_objects_size_change, created_ts)
        VALUES (%(bid)s, %(cid)s,  %(size)s, %(ts)s)
      """, bid=context.data['bid'], cid=context.data['cid'], size=size, ts=ts)
    helpers.assert_no_errcode(context)


@then(u'buckets usage contains rows')
def step_buckets_usage_contains_rows(context):
    result = []
    for db_conn in context.db_connects:
        db_usage = db_conn.get("""
            SELECT storage_class, byte_secs, size_change, start_ts FROM s3.buckets_usage WHERE bid=%(bid)s
        """, bid=context.data['bid'])
        for record in db_usage.records:
            result.append(record)
    expected = helpers.yaml.safe_load(context.text) or []
    helpers.compare_objects(expected, result, ignore_list_order=True, including=True)


@then(u'bucket usage at "{start_ts}" by shards')
def step_check_buckets_usage_by_shards(context, start_ts):
    result = []
    for db_conn in context.db_connects:
        db_usage = db_conn.get("""
            SELECT byte_secs, size_change FROM s3.buckets_usage
            WHERE bid=%(bid)s AND start_ts=%(start_ts)s
        """, bid=context.data['bid'], start_ts=start_ts)
        if db_usage.records:
            result.append(db_usage.records[0])
    expected = helpers.yaml.safe_load(context.text) or []
    helpers.compare_objects(expected, result, ignore_list_order=True)


@when(u'we change timestamps in chunks counters queue to "{ts}"')
def change_timestamps_chunk_counters_queue(context, ts):
    for db_conn in context.db_connects:
        db_conn.query("""
        UPDATE s3.chunks_counters_queue SET created_ts = %(ts)s WHERE bid = %(bid)s
      """, bid=context.data['bid'], ts=ts)
    helpers.assert_no_errcode(context)


@when(u'we change timestamps in chunks_counters to "{ts}"')
def change_timestamps_chunk_counters(context, ts):
    for db_conn in context.db_connects:
        db_conn.query("""
        UPDATE s3.chunks_counters SET updated_ts = %(ts)s
      """, ts=ts)
    helpers.assert_no_errcode(context)


@when(u'we change timestamps in chunks to "{ts}"')
def change_timestamps_chunks(context, ts):
    for db_conn in context.db_connects:
        db_conn.query("""
        UPDATE s3.chunks SET updated_ts = %(ts)s
      """, ts=ts)
    helpers.assert_no_errcode(context)

@when(u'we change timestamps in buckets_size to "{ts}"')
def change_timestamps_buckets_size(context, ts):
    for db_conn in context.db_connects:
        db_conn.query("""
        UPDATE s3.buckets_size SET target_ts = %(ts)s
      """, ts=ts)
    helpers.assert_no_errcode(context)


@when(u'create partition for "{ts}"')
def create_partitions_metadb(context, ts):
    for db_conn in context.meta_connects:
        db_conn.query("""
            select partman.create_partition_time('s3.buckets_usage', '{"%s"}')
        """ % ts)
    helpers.assert_no_errcode(context)


@then(u'buckets size contains rows')
def step_buckets_size_contains_rows(context):
    result = []
    for db_conn in context.meta_connects:
        db_usage = db_conn.get("""
            SELECT storage_class, size, target_ts FROM s3.buckets_size WHERE bid=%(bid)s
        """, bid=context.data['bid'])
        for record in db_usage.records:
            result.append(record)
    expected = helpers.yaml.safe_load(context.text) or []
    helpers.compare_objects(expected, result, ignore_list_order=True, including=True)


@when('we fill buckets size at "{ts}"')
def step_fill_buckets_size(context, ts):
    pgmeta_connstr = str(context.CONNSTRING['pgmeta'])
    for dbname, connstring in sorted(context.CONNSTRING.iteritems()):
        if dbname.startswith('s3meta'):
            helpers.run_script(context, 's3meta', 'fill_buckets_size', ['-d', connstring, '-p', pgmeta_connstr, '-t', ts])


@when(u'we update buckets size at "{ts}"')
def step_run_update_buckets_size_target(context, ts):
    pgmeta_connstr = str(context.CONNSTRING['pgmeta'])
    for dbname, connstring in sorted(context.CONNSTRING.iteritems()):
        if dbname.startswith('s3meta'):
            helpers.run_script(context, 's3meta', 'update_buckets_size', ['-d', connstring, '-p', pgmeta_connstr, '-t', ts])


@then('buckets sizes for "{ts1}" and "{ts2}" are the same')
def step_compare_buckets_size(context, ts1, ts2):
    query = "SELECT bid, shard_id, storage_class, size, target_ts s3.buckets_size WHERE target_ts = %(ts)s"
    for meta_conn in context.meta_connects:
        first = meta_conn.get(query, ts=ts1)
        second = meta_conn.get(query, ts=ts2)
        helpers.compare_objects(first.records, second.records, ignore_list_order=True)


@when(u'we delete buckets usage at "{ts}"')
def step_delete_buckets_usage(context, ts):
    for db_conn in context.meta_connects:
        db_conn.query("""
        DELETE FROM s3.buckets_usage WHERE start_ts=%(ts)s
      """, ts=ts)
    helpers.assert_no_errcode(context)

from __future__ import unicode_literals

import logging

from behave import then, when

import helpers

log = logging.getLogger(__name__)


@when('we break bucket stat for bucket "{bucket_name}"')
def step_break_stats_for_bucket(context, bucket_name):
    for conn in context.meta_connects:
        conn.query("""
            UPDATE s3.chunks_counters
            SET simple_objects_count = -simple_objects_count
            WHERE bid = (SELECT bid FROM s3.buckets WHERE name = %(bucket_name)s)
        """, bucket_name=bucket_name)


@then('we have "{count:d}" deleted objects of "{size:d}" size for bucket "{bucket_name}"')
def buckets_stats_deleted(context, count, size, bucket_name):
    deleted_objects_count = 0
    deleted_objects_size = 0
    for conn in context.meta_connects:
        conn_result = conn.get("""
            SELECT
              coalesce(sum(deleted_objects_count), 0) AS deleted_objects_count,
              coalesce(sum(deleted_objects_size), 0) AS deleted_objects_size
            FROM s3.chunks_counters
            WHERE bid = (SELECT bid FROM s3.buckets WHERE name = %(bucket_name)s)
        """, bucket_name=bucket_name)
        if conn_result.records:
            deleted_objects_count += conn_result.records[0]['deleted_objects_count']
            deleted_objects_size += conn_result.records[0]['deleted_objects_size']
    assert deleted_objects_count == count
    assert deleted_objects_size == size


@then('we have "{count:d}" active multipart upload(s) for bucket "{bucket_name}"')
def buckets_stats_active_multipart_upload(context, count, bucket_name):
    active_multipart_count = 0
    for conn in context.meta_connects:
        conn_result = conn.get("""
            SELECT
              coalesce(sum(active_multipart_count), 0) AS active_multipart_count
            FROM s3.chunks_counters
            WHERE bid = (SELECT bid FROM s3.buckets WHERE name = %(bucket_name)s)
        """, bucket_name=bucket_name)
        if conn_result.records:
            active_multipart_count += conn_result.records[0]['active_multipart_count']
    assert active_multipart_count == count


@then('we have zero counters for bucket "{bucket_name}"')
def buckets_zero_counters_objects(context, bucket_name):
    for conn in context.meta_connects:
        conn_result = conn.get("""
            SELECT
              count(*) AS not_zero_count
            FROM s3.chunks_counters
            WHERE bid = (SELECT bid FROM s3.buckets WHERE name = %(bucket_name)s)
              AND (simple_objects_count != 0 OR simple_objects_size != 0
                OR multipart_objects_count != 0 OR multipart_objects_size != 0
                OR objects_parts_count != 0 OR objects_parts_size != 0)
        """, bucket_name=bucket_name)
        assert conn_result.records[0]['not_zero_count'] == 0


@then('bucket stat for bucket "{bucket_name}"')
def buckets_stats_request_bucket_name(context, bucket_name):
    result = []
    for db_conn in context.meta_connects:
        buckets_stats = db_conn.get("""
            SELECT
                bid,
                name,
                service_id,
                CAST(0 AS INT) as storage_class,
                chunks_count,
                simple_objects_count,
                simple_objects_size,
                multipart_objects_count,
                multipart_objects_size,
                objects_parts_count,
                objects_parts_size,
                updated_ts,
                max_size
            FROM s3.bucket_stat
            WHERE name = %(bucket_name)s
        """, bucket_name=bucket_name)
        if buckets_stats.records:
            result.extend(buckets_stats.records)
    expected = helpers.yaml.load(context.text) or []
    helpers.compare_objects(expected, result, ignore_list_order=True)


@then('buckets stats for owner "{service_id:d}"')
def buckets_stats_request_service_id(context, service_id=None):
    result = []
    for db_conn in context.meta_connects:
        buckets_stats = db_conn.get("""
                SELECT
                    name,
                    service_id,
                    chunks_count,
                    simple_objects_count,
                    simple_objects_size,
                    multipart_objects_count,
                    multipart_objects_size,
                    objects_parts_count,
                    objects_parts_size,
                    updated_ts,
                    max_size
                FROM s3.bucket_stat
                WHERE (%(service_id)s IS NULL OR service_id = %(service_id)s)
            """, service_id=service_id)
        if buckets_stats.records:
            result.extend(buckets_stats.records)
    expected = helpers.yaml.load(context.text) or []
    helpers.compare_objects(expected, result, ignore_list_order=True)


@then('buckets stats for all buckets')
def buckets_stats_request(context):
    buckets_stats_request_service_id(context)

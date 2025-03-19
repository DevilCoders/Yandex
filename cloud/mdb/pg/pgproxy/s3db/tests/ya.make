PY2TEST()

OWNER(g:mdb g:s3)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/pg/pgproxy/s3db/recipe/recipe.inc)

DEPENDS(
    cloud/mdb/pg/pgproxy/s3db/scripts/s3db/check_chunks_counters
    cloud/mdb/pg/pgproxy/s3db/scripts/s3db/chunk_splitter
    cloud/mdb/pg/pgproxy/s3db/scripts/s3db/merge_chunks
    cloud/mdb/pg/pgproxy/s3db/scripts/s3db/purge_objects
    cloud/mdb/pg/pgproxy/s3db/scripts/s3db/update_chunks_counters
    cloud/mdb/pg/pgproxy/s3db/scripts/s3db/copy_delete_queue
    cloud/mdb/pg/pgproxy/s3db/scripts/s3meta/check_chunks_bounds
    cloud/mdb/pg/pgproxy/s3db/scripts/s3meta/chunk_creator
    cloud/mdb/pg/pgproxy/s3db/scripts/s3meta/chunk_mover
    cloud/mdb/pg/pgproxy/s3db/scripts/s3meta/chunk_purger
    cloud/mdb/pg/pgproxy/s3db/scripts/s3meta/fill_buckets_size
    cloud/mdb/pg/pgproxy/s3db/scripts/s3meta/finish_prepared_xacts
    cloud/mdb/pg/pgproxy/s3db/scripts/s3meta/update_buckets_size
    cloud/mdb/pg/pgproxy/s3db/scripts/s3meta/update_bucket_stat
    cloud/mdb/pg/pgproxy/s3db/scripts/s3meta/update_chunks_counters
    cloud/mdb/pg/pgproxy/s3db/scripts/s3meta/update_buckets_usage
    cloud/mdb/pg/pgproxy/s3db/scripts/s3meta/update_shard_stat
    cloud/mdb/pg/pgproxy/s3db/scripts/s3_closer
)

PEERDIR(
    library/python/testing/behave
    contrib/python/psycopg2
    contrib/python/PyYAML
    contrib/python/tenacity
)

SIZE(MEDIUM)

FORK_SUBTESTS()

SPLIT_FACTOR(64)

REQUIREMENTS(
    cpu:4
    ram:12
)

NO_CHECK_IMPORTS(
    behave.*
)

END()

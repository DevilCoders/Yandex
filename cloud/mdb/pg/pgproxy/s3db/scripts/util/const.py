#!/usr/bin/env python
# encoding: utf-8

CHUNKS_COUNTERS_ADVISORY_LOCK = -42
CHUNK_MOVER_APPLICATION_NAME = 's3_script_chunk_mover'
CHUNK_SPLITTER_APPLICATION_NAME = 's3_script_chunk_splitter'
CHUNK_MERGER_APPLICATION_NAME = 's3_script_chunk_merger'
SMART_MOVER_APPLICATION_NAME = 's3_script_smart_mover'
UPDATE_META_CHUNKS_COUNTERS_APPLICATION_NAME = 's3_script_update_meta_chunks_counters'
UPDATE_DB_CHUNKS_COUNTERS_APPLICATION_NAME = 's3_script_update_db_chunks_counters'
UPDATE_BUCKETS_USAGE_APPLICATION_NAME = 's3_script_update_buckets_usage'
CHECK_META_CHUNKS_COUNTERS_APPLICATION_NAME = 's3_script_check_meta_chunks_counters'
CHECK_DB_CHUNKS_COUNTERS_APPLICATION_NAME = 's3_script_check_db_chunks_counters'
CA_CERTS_PATH = '/opt/yandex/allCAs.pem'
GET_LOCK_RETRY_COUNT = 3

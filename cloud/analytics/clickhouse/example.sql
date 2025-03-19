CREATE TABLE %(table_name)s (timestamp Int64, date Date) ENGINE = ReplicatedMergeTree(
  '/clickhouse/tables/{shard}/%(table_name)s',
  '{replica}',
  date,
  timestamp,
  8192
)

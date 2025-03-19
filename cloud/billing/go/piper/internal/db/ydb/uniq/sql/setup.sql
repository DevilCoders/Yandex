CREATE TABLE `test/uniques/hashed` (
    key_hash uint64,
    source_hash uint64,
    chk uint32,
    expire_at date,

    PRIMARY KEY (key_hash)
) WITH (
    TTL = Interval("P1D") ON expire_at,
    KEY_BLOOM_FILTER = enabled,
    AUTO_PARTITIONING_BY_SIZE = enabled,
    AUTO_PARTITIONING_MIN_PARTITIONS_COUNT = 32,
    AUTO_PARTITIONING_MAX_PARTITIONS_COUNT = 2048,
    UNIFORM_PARTITIONS = 32
);

-- FIXME Regular "object_parts" should be renamed to "incomplete_parts" at some point
-- and this "completed_parts" table should be renamed to "object_parts"
CREATE TABLE s3.completed_parts
(
    bid             uuid                      NOT NULL,
    name            text COLLATE "C"          NOT NULL,
    object_created  timestamptz               NOT NULL,
    -- part_id will be removed in the future if we decide to implement PATCH so we don't index on it
    -- but as of now the code still requires it so it's still here for the first part
    part_id         integer                   NOT NULL,
    -- end (not start) offset because we need lookups on end_offset > START and (end_offset-data_size) < END
    end_offset      bigint                    NOT NULL,
    created         timestamptz DEFAULT now() NOT NULL,
    data_size       bigint      DEFAULT 0     NOT NULL,
    data_md5        uuid                      NOT NULL,
    mds_couple_id   integer                   NOT NULL,
    mds_key_version integer                   NOT NULL,
    mds_key_uuid    uuid                      NOT NULL,
    storage_class   integer,
    encryption      text COLLATE "C",
    PRIMARY KEY (bid, name, object_created, end_offset),
    CONSTRAINT check_size CHECK (
        (data_size >= 0 AND data_size <= 5368709120) -- 5GB
    )
) PARTITION BY HASH (bid);

-- 32 partitions because 256 seems too much: bucket-based hash partitioning is highly non-uniform
CREATE TABLE s3.completed_parts_0 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 0);
CREATE TABLE s3.completed_parts_1 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 1);
CREATE TABLE s3.completed_parts_2 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 2);
CREATE TABLE s3.completed_parts_3 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 3);
CREATE TABLE s3.completed_parts_4 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 4);
CREATE TABLE s3.completed_parts_5 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 5);
CREATE TABLE s3.completed_parts_6 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 6);
CREATE TABLE s3.completed_parts_7 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 7);
CREATE TABLE s3.completed_parts_8 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 8);
CREATE TABLE s3.completed_parts_9 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 9);
CREATE TABLE s3.completed_parts_10 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 10);
CREATE TABLE s3.completed_parts_11 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 11);
CREATE TABLE s3.completed_parts_12 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 12);
CREATE TABLE s3.completed_parts_13 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 13);
CREATE TABLE s3.completed_parts_14 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 14);
CREATE TABLE s3.completed_parts_15 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 15);
CREATE TABLE s3.completed_parts_16 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 16);
CREATE TABLE s3.completed_parts_17 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 17);
CREATE TABLE s3.completed_parts_18 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 18);
CREATE TABLE s3.completed_parts_19 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 19);
CREATE TABLE s3.completed_parts_20 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 20);
CREATE TABLE s3.completed_parts_21 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 21);
CREATE TABLE s3.completed_parts_22 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 22);
CREATE TABLE s3.completed_parts_23 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 23);
CREATE TABLE s3.completed_parts_24 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 24);
CREATE TABLE s3.completed_parts_25 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 25);
CREATE TABLE s3.completed_parts_26 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 26);
CREATE TABLE s3.completed_parts_27 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 27);
CREATE TABLE s3.completed_parts_28 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 28);
CREATE TABLE s3.completed_parts_29 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 29);
CREATE TABLE s3.completed_parts_30 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 30);
CREATE TABLE s3.completed_parts_31 PARTITION OF s3.completed_parts FOR VALUES WITH (MODULUS 32, REMAINDER 31);

ALTER TABLE s3.objects ADD CONSTRAINT check_data_1 CHECK (
    -- Old "check_data"
    (
        (delete_marker IS true AND mds_couple_id IS NULL
            AND mds_key_version IS NULL AND mds_key_uuid IS NULL
            AND data_size IS NULL AND data_md5 IS NULL
            AND parts_count IS NULL AND parts IS NULL)
        OR
        (delete_marker IS false AND mds_couple_id IS NOT NULL
            AND mds_key_version IS NOT NULL AND mds_key_uuid IS NOT NULL
            AND data_size IS NOT NULL AND data_md5 IS NOT NULL
            AND parts_count IS NULL AND parts IS NULL)
        OR
        -- Empty object could be not uploaded to MDS
        (delete_marker IS false AND mds_couple_id IS NULL
            AND mds_key_version IS NULL AND mds_key_uuid IS NULL
            AND data_size = 0 and data_md5 IS NOT NULL
            AND parts_count IS NULL AND parts IS NULL)
        OR
        -- Multipart object could have no "metadata" record in MDS
        (delete_marker IS false AND mds_couple_id IS NULL
            AND mds_key_version IS NULL AND mds_key_uuid IS NULL
            AND data_size IS NOT NULL AND data_md5 IS NOT NULL
            AND parts_count IS NOT NULL)
        OR
        -- Multipart object with "metadata" record in MDS
        (delete_marker IS false AND mds_couple_id IS NOT NULL
            AND mds_key_version IS NOT NULL AND mds_key_uuid IS NOT NULL
            AND data_size IS NOT NULL AND data_md5 IS NOT NULL
            AND parts_count IS NOT NULL)
        OR
        -- Object migrated from YDB
        (delete_marker IS false
            AND metadata IS NOT NULL AND (metadata->>'migrated')::bool IS true
            )
    )
    -- Old "check_parts"
    AND
    (
        (parts_count IS NULL AND parts IS NULL)
        OR
        (parts_count IS NOT NULL AND (parts IS NULL OR
            array_length(parts, 1) >= 1
            AND array_length(parts, 1) <= 10000
            AND array_length(parts, 1) = parts_count))
        OR
        -- object migrated from YDB
        (metadata IS NOT NULL AND (metadata->>'migrated')::bool IS true)
    )
    -- Old "check_size"
    AND
    (
        (data_size IS NULL)
        OR
        -- single part (simple) object's size is limited by 5GB
        (data_size IS NOT NULL AND parts_count IS NULL
            AND data_size >= 0
            AND data_size <= 5368709120)
        OR
        -- multipart object's size limit is inherited from s3.object_parts
        (data_size IS NOT NULL AND parts_count IS NOT NULL
            AND data_size >= 0)
        OR
        -- object migrated from YDB
        (metadata IS NOT NULL AND (metadata->>'migrated')::bool IS true)
    )
) NOT VALID;

ALTER TABLE s3.objects_noncurrent ADD CONSTRAINT check_data_1 CHECK (
    -- Old "check_data"
    (
        (delete_marker IS true AND mds_couple_id IS NULL
            AND mds_key_version IS NULL AND mds_key_uuid IS NULL
            AND data_size IS NULL AND data_md5 IS NULL
            AND parts_count IS NULL AND parts IS NULL)
        OR
        (delete_marker IS false AND mds_couple_id IS NOT NULL
            AND mds_key_version IS NOT NULL AND mds_key_uuid IS NOT NULL
            AND data_size IS NOT NULL AND data_md5 IS NOT NULL
            AND parts_count IS NULL AND parts IS NULL)
        OR
        -- Empty object could be not uploaded to MDS
        (delete_marker IS false AND mds_couple_id IS NULL
            AND mds_key_version IS NULL AND mds_key_uuid IS NULL
            AND data_size = 0 and data_md5 IS NOT NULL
            AND parts_count IS NULL AND parts IS NULL)
        OR
        -- Multipart object could have no "metadata" record in MDS
        (delete_marker IS false AND mds_couple_id IS NULL
            AND mds_key_version IS NULL AND mds_key_uuid IS NULL
            AND data_size IS NOT NULL AND data_md5 IS NOT NULL
            AND parts_count IS NOT NULL)
        OR
        -- Multipart object with "metadata" record in MDS
        (delete_marker IS false AND mds_couple_id IS NOT NULL
            AND mds_key_version IS NOT NULL AND mds_key_uuid IS NOT NULL
            AND data_size IS NOT NULL AND data_md5 IS NOT NULL
            AND parts_count IS NOT NULL)
        OR
        -- object migrated from YDB
        (metadata IS NOT NULL AND (metadata->>'migrated')::bool IS true)
    )
    -- Old "check_parts"
    AND
    (
        (parts_count IS NULL AND parts IS NULL)
        OR
        (parts_count IS NOT NULL AND (parts IS NULL OR
            array_length(parts, 1) >= 1
            AND array_length(parts, 1) <= 10000
            AND array_length(parts, 1) = parts_count))
        OR
        -- object migrated from YDB
        (metadata IS NOT NULL AND (metadata->>'migrated')::bool IS true)
    )
    -- Old "check_size"
    AND
    (
        (data_size IS NULL)
        OR
        -- single part (simple) object's size is limited by 5GB
        (data_size IS NOT NULL AND parts_count IS NULL
            AND data_size >= 0
            AND data_size <= 5368709120)
        OR
        -- multipart object's size limit is inherited from s3.object_parts
        (data_size IS NOT NULL AND parts_count IS NOT NULL
            AND data_size >= 0)
        OR
        -- object migrated from YDB
        (metadata IS NOT NULL AND (metadata->>'migrated')::bool IS true)
    )
) NOT VALID;

-- Mark constraints validated without actually checking all data rows :-)
-- We know for sure the new constraint is always valid where the old one is valid
-- Validating them in a straight way would take a LOT of time
UPDATE pg_constraint ct
SET convalidated=true
FROM pg_class cl, pg_namespace ns
WHERE ct.conname='check_data_1' AND ct.conrelid=cl.oid
    AND cl.relname ~ '^objects(_noncurrent)?(_\d+)?$' AND cl.relnamespace=ns.oid
    AND ns.nspname='s3';

-- Drop old constraints
ALTER TABLE s3.objects DROP CONSTRAINT check_data;
ALTER TABLE s3.objects DROP CONSTRAINT check_parts;
ALTER TABLE s3.objects DROP CONSTRAINT check_size;
ALTER TABLE s3.objects_noncurrent DROP CONSTRAINT check_data;
ALTER TABLE s3.objects_noncurrent DROP CONSTRAINT check_parts;
ALTER TABLE s3.objects_noncurrent DROP CONSTRAINT check_size;

-- Rename constraints back to check_data
ALTER TABLE s3.objects RENAME CONSTRAINT check_data_1 TO check_data;
ALTER TABLE s3.objects_noncurrent RENAME CONSTRAINT check_data_1 TO check_data;

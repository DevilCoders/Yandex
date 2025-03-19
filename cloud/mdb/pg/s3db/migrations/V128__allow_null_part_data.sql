ALTER TABLE s3.object_parts ADD CONSTRAINT check_data_1 CHECK (
    -- "Root" record with no "metadata" record in MDS
    (part_id = 0 AND mds_couple_id IS NULL
        AND mds_key_version IS NULL AND mds_key_uuid IS NULL
        AND data_md5 IS NULL)
    OR
    -- "Root" record with "metadata" record in MDS
    (part_id = 0 AND mds_couple_id IS NOT NULL
        AND mds_key_version IS NOT NULL AND mds_key_uuid IS NOT NULL
        AND data_md5 IS NULL)
    OR
    -- Normal part, may contain empty MDS ID if data size is 0 (allowed in S3)
    (part_id > 0 AND part_id <= 10000 AND
        (data_size = 0 OR mds_couple_id IS NOT NULL
            AND mds_key_version IS NOT NULL AND mds_key_uuid IS NOT NULL)
        AND data_md5 IS NOT NULL)
) NOT VALID;

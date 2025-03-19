CREATE UNIQUE INDEX CONCURRENTLY idx_object_parts ON s3.object_parts
    (bid, name, object_created, part_id);

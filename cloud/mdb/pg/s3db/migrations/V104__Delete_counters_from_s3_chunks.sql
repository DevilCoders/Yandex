ALTER TABLE s3.chunks
    DROP COLUMN simple_objects_count,
    DROP COLUMN simple_objects_size,
    DROP COLUMN multipart_objects_count,
    DROP COLUMN multipart_objects_size,
    DROP COLUMN objects_parts_count,
    DROP COLUMN objects_parts_size;

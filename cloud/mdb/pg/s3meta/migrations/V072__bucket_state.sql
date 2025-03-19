CREATE TYPE s3.bucket_state AS ENUM (
    'alive',
    'deleting'
);

ALTER TABLE ONLY s3.buckets ADD COLUMN
    state s3.bucket_state NOT NULL DEFAULT 'alive';

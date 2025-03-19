CREATE SCHEMA IF NOT EXISTS s3;

CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TYPE s3.bucket_versioning_type AS ENUM (
    'disabled',
    'enabled',
    'suspended'
);

CREATE TABLE s3.buckets
(
    bid         uuid    NOT NULL DEFAULT gen_random_uuid(),
    name        text COLLATE "C" NOT NULL,
    created     timestamp with time zone  NOT NULL DEFAULT current_timestamp,
    versioning  s3.bucket_versioning_type NOT NULL DEFAULT 'disabled',
    banned      boolean NOT NULL DEFAULT false,
    CONSTRAINT pk_buckets PRIMARY KEY (bid),
    CONSTRAINT check_bucket_name CHECK (
        char_length(name) >= 3 AND
        char_length(name) < 64
    )
);

CREATE UNIQUE INDEX uk_buckets_name ON s3.buckets (name);

CREATE SEQUENCE s3.cid_seq START WITH 1;

CREATE TABLE s3.chunks
(
    bid         uuid    NOT NULL,
    cid         bigint  NOT NULL DEFAULT nextval('s3.cid_seq'),
    created     timestamp with time zone  NOT NULL DEFAULT current_timestamp,
    read_only   boolean NOT NULL DEFAULT false,
    start_key   text,
    end_key     text,
    CONSTRAINT pk_chunks PRIMARY KEY (bid, cid),
    CONSTRAINT fk_chunks_bid_buckets FOREIGN KEY (bid)
        REFERENCES s3.buckets ON DELETE CASCADE,
    CONSTRAINT check_key_ranges CHECK (
        (start_key IS NULL AND end_key IS NULL)
        OR
        (start_key IS NULL
            AND end_key IS NOT NULL
            AND int4range(1, 1025) @> octet_length(convert_to(end_key, 'UTF8')))
        OR
        (start_key IS NOT NULL
            AND end_key IS NULL
            AND int4range(1, 1025) @> octet_length(convert_to(start_key, 'UTF8')))
        OR
        (start_key IS NOT NULL
            AND end_key IS NOT NULL
            AND int4range(1, 1025) @> octet_length(convert_to(start_key, 'UTF8'))
            AND int4range(1, 1025) @> octet_length(convert_to(end_key, 'UTF8')))
    )
);

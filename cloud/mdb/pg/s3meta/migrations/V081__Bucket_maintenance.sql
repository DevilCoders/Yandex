CREATE TABLE s3.bucket_maintenance
(
    bid                     uuid NOT NULL,
    objects_maintained_at   timestamp with time zone,

    CONSTRAINT pk_bucket_maintenance PRIMARY KEY (bid)
);

CREATE TABLE s3.buckets_usage_processing
(
    bid         uuid NOT NULL,
    start_ts    timestamp with time zone NOT NULL,
    status      int NOT NULL,
    job_id      text,
    process_ts  timestamp with time zone NOT NULL DEFAULT current_timestamp,
    PRIMARY KEY (bid, start_ts)
);
CREATE INDEX ON s3.buckets_usage_processing(status, job_id);

CREATE TABLE s3.buckets_history
(
    bid         uuid NOT NULL PRIMARY KEY,
    name        text COLLATE "C" NOT NULL,
    created     timestamp with time zone NOT NULL,
    deleted     timestamp with time zone,
    service_id  bigint NOT NULL
);
CREATE INDEX ON s3.buckets_history(created);

INSERT INTO s3.buckets_history(bid, name, created, service_id)
SELECT bid, name, created, service_id
FROM s3.buckets;

CREATE TABLE s3.buckets_usage_processing_timestamps
(
    start_ts  timestamp with time zone NOT NULL PRIMARY KEY
);

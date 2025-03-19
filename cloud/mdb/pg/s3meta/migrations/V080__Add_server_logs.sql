CREATE TABLE s3.server_logs
(
    src_bid            uuid NOT NULL,
    src_bucket_name    text NOT NULL,

    setup JSONB,

    CONSTRAINT pk_server_logs_src_bid PRIMARY KEY (src_bid)
);

CREATE TABLE s3.server_logs_tasks
(
    time_stamp timestamp with time zone NOT NULL,
    metainfo   JSONB,

    interval_span_type int NOT NULL,

    task_status int NOT NULL,
    worker_id   text,

    CONSTRAINT pk_server_logs_tasks PRIMARY KEY (time_stamp, interval_span_type)
);

CREATE INDEX idx_server_logs_tasks_status ON s3.server_logs_tasks(task_status, worker_id);

CREATE TABLE s3.server_logs_tasks_buckets
(
    -- Logical foreign key to `s3.server_logs_tasks` times_stamp field.
    time_stamp  timestamp with time zone NOT NULL,

    src_bid            uuid NOT NULL,
    src_bucket_name    text NOT NULL,

    setup JSONB,

    CONSTRAINT pk_server_logs_src_dst_bid PRIMARY KEY (time_stamp, src_bid)
);

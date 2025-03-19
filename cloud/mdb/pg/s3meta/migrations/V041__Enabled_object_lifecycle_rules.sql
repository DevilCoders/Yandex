CREATE TABLE s3.enabled_lc_rules
(
    bid         uuid,
    started_ts  timestamptz NOT NULL DEFAULT current_timestamp,
    finished_ts timestamptz DEFAULT NULL,
    rules       JSONB NOT NULL,
    CONSTRAINT pk_update_id PRIMARY KEY (bid, started_ts)
);

CREATE TABLE s3.lifecycle_scheduling_status
(
    key                   BIGINT NOT NULL PRIMARY KEY,
    scheduling_ts         timestamptz NOT NULL,
    last_processed_bucket TEXT DEFAULT NULL,
    status                int DEFAULT 0
 );

CREATE INDEX idx_lifecycle_scheduling_status_scheduling_ts ON s3.lifecycle_scheduling_status USING btree (scheduling_ts);

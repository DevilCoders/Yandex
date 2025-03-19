CREATE TABLE s3.maintenance_queue
(
    bid              uuid    NOT NULL,
    name             text    NOT NULL,
    created          timestamp with time zone NOT NULL,
    process_after    timestamp with time zone NOT NULL DEFAULT current_timestamp
);

CREATE UNIQUE INDEX pk_maintenance_queue ON s3.maintenance_queue (bid, name, created);

CREATE INDEX i_maintenance_queue_ts ON s3.maintenance_queue (process_after ASC);

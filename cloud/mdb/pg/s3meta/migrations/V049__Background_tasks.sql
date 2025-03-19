CREATE TABLE s3.background_tasks
(
    bid        uuid,
    task_id    uuid,
    status     int,
    created    timestamptz,
    modified   timestamptz,
    created_by uuid,
    type       int,
    params     jsonb,
    CONSTRAINT pk_bid_task_id PRIMARY KEY (bid, task_id)
);

CREATE INDEX idx_background_tasks_listing ON s3.background_tasks USING btree (bid, created, status);

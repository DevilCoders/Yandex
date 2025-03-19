CREATE SEQUENCE s3.maintenance_tasks_id START WITH 1;

CREATE TABLE s3.maintenance_tasks
(
    task_id                 bigint NOT NULL default nextval('s3.maintenance_tasks_id'),
    task_type               int NOT NULL,
    task_created            timestamp with time zone NOT NULL default current_timestamp,
    settings                JSONB,
    priority                int NOT NULL default 0,

    last_processed_bucket   text,

    status                  int NOT NULL default 0,
    status_changed_at       timestamp with time zone NOT NULL default current_timestamp,

    scheduled_count         int NOT NULL default 0,
    remaining_count         int NOT NULL default 0
);

-- pk, task ID for all child tables, select earliest tasks
CREATE UNIQUE INDEX pk_maintenance_tasks ON s3.maintenance_tasks (task_id);

-- select new tasks
CREATE INDEX i_maintenance_tasks_status ON s3.maintenance_tasks (task_type, status, priority DESC, task_created ASC);


DROP TABLE s3.bucket_maintenance;
CREATE TABLE s3.bucket_maintenance_queue
(
    task_id                 bigint NOT NULL,
    task_type               int NOT NULL,
    task_created            timestamp with time zone NOT NULL default current_timestamp,
    settings                JSONB,
    priority                int NOT NULL default 0,

    bid                     uuid NOT NULL,
    name                    text COLLATE "C" NOT NULL,

    status                  int NOT NULL default 0,
    status_changed_at       timestamp with time zone NOT NULL default current_timestamp,

    scheduled_count         int NOT NULL default 0,
    remaining_count         int NOT NULL default 0,

    process_after           timestamp with time zone NOT NULL default current_timestamp
);

-- pk, select rows by task ID
CREATE UNIQUE INDEX pk_bucket_maintenance_queue ON s3.bucket_maintenance_queue (task_id, bid);

-- select new tasks
CREATE INDEX i_bucket_maintenance_queue_status ON s3.bucket_maintenance_queue (task_type, status, priority DESC, process_after ASC);


CREATE TABLE s3.chunk_maintenance_queue
(
    task_id                 bigint NOT NULL,
    task_type               int NOT NULL,
    task_created            timestamp with time zone NOT NULL default current_timestamp,
    settings                JSONB,
    priority                int NOT NULL default 0,

    bid                     uuid NOT NULL,
    start_key               text,
    end_key                 text,
    last_processed_key      text,

    status                  int NOT NULL default 0,
    status_changed_at       timestamp with time zone NOT NULL default current_timestamp,

    scheduled_count         int NOT NULL default 0,
    remaining_count         int NOT NULL default 0,

    process_after           timestamp with time zone NOT NULL default current_timestamp
);

-- pk, select rows by task ID
CREATE UNIQUE INDEX pk_chunk_maintenance_queue ON s3.chunk_maintenance_queue (task_id, bid, start_key ASC NULLS FIRST, end_key ASC NULLS LAST);

-- select new tasks
CREATE INDEX i_chunk_maintenance_queue_status ON s3.chunk_maintenance_queue (task_type, status, priority DESC, process_after ASC);

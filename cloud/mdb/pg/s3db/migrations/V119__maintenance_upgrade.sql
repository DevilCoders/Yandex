ALTER TABLE s3.maintenance_queue ADD COLUMN task_id bigint NOT NULL default 0;
ALTER TABLE s3.maintenance_queue ADD COLUMN task_type int NOT NULL default 0;
ALTER TABLE s3.maintenance_queue ADD COLUMN task_created timestamp with time zone NOT NULL default current_timestamp;
ALTER TABLE s3.maintenance_queue ADD COLUMN priority int NOT NULL default 0;
ALTER TABLE s3.maintenance_queue ADD COLUMN null_version boolean NOT NULL default true;

-- pk, select rows by task ID
CREATE UNIQUE INDEX pk_maintenance_queue ON s3.maintenance_queue (task_id, bid, name, created);

-- select new tasks
CREATE INDEX i_maintenance_queue_status ON s3.maintenance_queue (task_type, priority DESC, process_after ASC);

CREATE TABLE IF NOT EXISTS cs_background_tasks (
    sequence_task_id BIGSERIAL NOT NULL PRIMARY KEY,
    internal_task_id TEXT NOT NULL,
    class_name TEXT NOT NULL,
    data bytea NOT NULL,
    owner_id TEXT NOT NULL,
    queue_id TEXT NOT NULL,
    construction_instant INTEGER NOT NULL,
    start_instant INTEGER NOT NULL,
    last_ping_instant INTEGER,
    current_host TEXT
);

DO $$ BEGIN
    IF NOT EXISTS (SELECT constraint_name FROM information_schema.table_constraints WHERE table_name = 'cs_background_tasks' AND constraint_name = 'cs_background_tasks_queue_id_owner_id_internal_task_id_unique') THEN
        ALTER TABLE cs_background_tasks ADD CONSTRAINT cs_background_tasks_queue_id_owner_id_internal_task_id_unique UNIQUE (queue_id, owner_id, internal_task_id);
    END IF;
END $$;


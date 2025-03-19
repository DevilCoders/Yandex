CREATE TABLE IF NOT EXISTS preparation_markers (
    table_name TEXT NOT NULL,
    object_id TEXT NOT NULL,
    action_id TEXT NOT NULL
);

CREATE UNIQUE INDEX IF NOT EXISTS preparation_markers_unique ON preparation_markers(table_name, object_id, action_id);
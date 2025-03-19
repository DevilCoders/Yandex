CREATE TABLE IF NOT EXISTS cs_resources (
    resource_id SERIAL NOT NULL UNIQUE,
    resource_key TEXT NOT NULL PRIMARY KEY,
    access_id TEXT,
    revision SERIAL NOT NULL UNIQUE,
    deadline INTEGER,
    class_name TEXT NOT NULL,
    container bytea
);

CREATE TABLE IF NOT EXISTS cs_resources_history (
    history_event_id BIGSERIAL PRIMARY KEY,
    history_user_id text NOT NULL,
    history_originator_id text,
    history_action text NOT NULL,
    history_timestamp integer NOT NULL,
    history_comment text,

    resource_id SERIAL NOT NULL,
    resource_key TEXT NOT NULL,
    access_id TEXT,
    revision SERIAL NOT NULL,
    deadline INTEGER,
    class_name TEXT NOT NULL,
    container bytea
);

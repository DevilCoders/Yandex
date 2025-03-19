CREATE TABLE IF NOT EXISTS snc_groups (
    group_id TEXT NOT NULL PRIMARY KEY,
    default_snapshot_id INTEGER,
    revision SERIAL NOT NULL
);


CREATE TABLE IF NOT EXISTS snc_groups_history (
    history_event_id BIGSERIAL PRIMARY KEY,
    history_user_id text NOT NULL,
    history_originator_id text,
    history_action text NOT NULL,
    history_timestamp integer NOT NULL,
    history_comment text,

    group_id TEXT NOT NULL,
    default_snapshot_id INTEGER,
    revision SERIAL NOT NULL
);

CREATE TABLE IF NOT EXISTS snc_snapshots (
    snapshot_code TEXT NOT NULL UNIQUE,
    snapshot_id SERIAL PRIMARY KEY,
    snapshot_group_id TEXT NOT NULL,
    revision SERIAL NOT NULL,
    enabled BOOLEAN NOT NULL DEFAULT(true),
    status TEXT NOT NULL,
    last_status_modification INTEGER NOT NULL,
    storage_id TEXT,
    content_fetcher json NOT NULL,
    fetching_context json NOT NULL,
    FOREIGN KEY (snapshot_group_id) REFERENCES snc_groups(group_id)
);


CREATE TABLE IF NOT EXISTS snc_snapshots_history (
    history_event_id BIGSERIAL PRIMARY KEY,
    history_user_id text NOT NULL,
    history_originator_id text,
    history_action text NOT NULL,
    history_timestamp integer NOT NULL,
    history_comment text,

    snapshot_code TEXT NOT NULL,
    snapshot_id SERIAL NOT NULL,
    snapshot_group_id TEXT NOT NULL,
    revision SERIAL NOT NULL,
    enabled BOOLEAN NOT NULL,
    status TEXT NOT NULL,
    last_status_modification INTEGER NOT NULL,
    storage_id TEXT,
    content_fetcher json NOT NULL,
    fetching_context json NOT NULL
);

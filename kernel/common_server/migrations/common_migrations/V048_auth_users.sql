CREATE TABLE IF NOT EXISTS users (
    link_id SERIAL NOT NULL PRIMARY KEY,
    user_name text NOT NULL,
    auth_user_id TEXT NOT NULL,
    auth_module_id TEXT,
    UNIQUE(auth_module_id, auth_user_id)
);

CREATE TABLE IF NOT EXISTS users_history (
    history_event_id BIGSERIAL PRIMARY KEY,
    history_user_id text NOT NULL,
    history_originator_id text,
    history_action text NOT NULL,
    history_timestamp integer NOT NULL,
    history_comment text,

    link_id INTEGER NOT NULL,
    user_name text NOT NULL,
    auth_user_id TEXT NOT NULL,
    auth_module_id TEXT
); 
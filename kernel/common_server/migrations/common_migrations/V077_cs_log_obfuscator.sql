CREATE TABLE IF NOT EXISTS cs_obfuscators (
    obfuscator_id SERIAL PRIMARY Key,
    name text NOT NULL UNIQUE,
    class_name TEXT NOT NULL,
    priority INTEGER NOT NULL,
    revision SERIAL NOT NULL UNIQUE,
    data json
);

CREATE TABLE IF NOT EXISTS cs_obfuscators_history (
    history_event_id BIGSERIAL PRIMARY KEY,
    history_user_id text NOT NULL,
    history_originator_id text,
    history_action text NOT NULL,
    history_timestamp integer NOT NULL,
    history_comment text,

    obfuscator_id INTEGER NOT NULL,
    name text NOT NULL,
    class_name TEXT NOT NULL,
    priority INTEGER NOT NULL,
    revision INTEGER NOT NULL,
    data json
);

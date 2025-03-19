CREATE TABLE IF NOT EXISTS cs_secrets (
    secret_id uuid PRIMARY KEY DEFAULT (uuid_generate_v4()),
    secret_data bytea NOT NULL,
    secret_data_hash TEXT NOT NULL,
    secret_data_type TEXT NOT NULL
);


CREATE TABLE IF NOT EXISTS cs_secrets_history (
    history_event_id BIGSERIAL PRIMARY KEY,
    history_user_id text NOT NULL,
    history_originator_id text,
    history_action text NOT NULL,
    history_timestamp integer NOT NULL,
    history_comment text,

    secret_id uuid NOT NULL,
    secret_data bytea NOT NULL,
    secret_data_hash TEXT NOT NULL,
    secret_data_type TEXT NOT NULL
);

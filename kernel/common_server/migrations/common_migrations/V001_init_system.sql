CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

CREATE TABLE IF NOT EXISTS server_settings (
    setting_key text NOT NULL,
    setting_subkey text DEFAULT '' NOT NULL,
    setting_value text NOT NULL
);


CREATE TABLE IF NOT EXISTS server_settings_history (
    history_event_id BIGSERIAL PRIMARY KEY,
    history_user_id text NOT NULL,
    history_originator_id text,
    history_action text NOT NULL,
    history_timestamp integer NOT NULL,
    history_comment text,

    setting_key text NOT NULL,
    setting_subkey text DEFAULT '' NOT NULL,
    setting_value text NOT NULL
);

CREATE TABLE IF NOT EXISTS rt_background_settings (
    bp_id uuid DEFAULT (uuid_generate_v4()) NOT NULL UNIQUE,
    bp_name TEXT NOT NULL UNIQUE,
    bp_type TEXT NOT NULL,
    bp_settings TEXT NOT NULL,
    bp_revision BIGSERIAL,
    bp_enabled BOOLEAN DEFAULT (false)
);

CREATE TABLE IF NOT EXISTS rt_background_settings_history (
    history_event_id SERIAL PRIMARY KEY,
    history_user_id text NOT NULL,
    history_action text NOT NULL,
    history_timestamp integer NOT NULL,
    history_comment text,
    history_originator_id text,

    bp_id uuid NOT NULL,
    bp_name TEXT NOT NULL,
    bp_type TEXT NOT NULL,
    bp_settings TEXT NOT NULL,
    bp_revision BIGINT,
    bp_enabled BOOLEAN NOT NULL
);


CREATE TABLE IF NOT EXISTS rt_background_state (
    bp_name TEXT NOT NULL UNIQUE,
    bp_type TEXT NOT NULL,
    bp_state TEXT NOT NULL,
    bp_last_execution INTEGER DEFAULT 0,
    bp_status TEXT DEFAULT 'NOT_STARTED'
);

CREATE TABLE IF NOT EXISTS db_event_log (
    ctype TEXT,
    event TEXT,
    host TEXT,
    instant BIGINT,
    duration BIGINT,
    revision BIGINT,
    service TEXT
);

CREATE TABLE IF NOT EXISTS tag_descriptions (
    name text PRIMARY KEY,
    class_name TEXT NOT NULL,
    tag_description_object TEXT,
    deprecated boolean DEFAULT(false),
    description text,
    explicit_unique_policy text,
    UNIQUE(name)
);


CREATE TABLE IF NOT EXISTS tag_descriptions_history (
    history_event_id BIGSERIAL PRIMARY KEY,
    history_user_id text NOT NULL,
    history_originator_id text,
    history_action text NOT NULL,
    history_timestamp integer NOT NULL,
    history_comment text,

    name text NOT NULL,
    class_name TEXT NOT NULL,
    tag_description_object TEXT,
    deprecated boolean,
    description text,
    explicit_unique_policy text
);


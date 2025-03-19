CREATE TABLE IF NOT EXISTS roles (
    role_name TEXT NOT NULL PRIMARY KEY,
    revision SERIAL,
    role_id SERIAL NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS roles_history (
    history_event_id BIGSERIAL PRIMARY KEY,
    history_user_id text NOT NULL,
    history_originator_id text,
    history_action text NOT NULL,
    history_timestamp integer NOT NULL,
    history_comment text,

    role_name TEXT NOT NULL,
    revision INTEGER NOT NULL,
    role_id SERIAL NOT NULL
);

CREATE TABLE IF NOT EXISTS item_permissions (
    item_id TEXT NOT NULL PRIMARY KEY,
    class_name TEXT NOT NULL,
    revision SERIAL UNIQUE,
    item_priority INTEGER NOT NULL DEFAULT(0),
    item_enabled BOOLEAN NOT NULL DEFAULT(true),
    data JSON
);

CREATE TABLE IF NOT EXISTS item_permissions_history (
    history_event_id BIGSERIAL PRIMARY KEY,
    history_user_id text NOT NULL,
    history_originator_id text,
    history_action text NOT NULL,
    history_timestamp integer NOT NULL,
    history_comment text,

    item_id TEXT NOT NULL,
    class_name TEXT NOT NULL,
    revision INTEGER NOT NULL,
    item_priority INTEGER NOT NULL,
    item_enabled BOOLEAN NOT NULL,
    data JSON
);

CREATE TABLE IF NOT EXISTS links_role_item (
    link_id SERIAL NOT NULL PRIMARY KEY,
    slave_id TEXT NOT NULL,
    owner_id TEXT NOT NULL,
    FOREIGN KEY (slave_id) REFERENCES item_permissions(item_id),
    FOREIGN KEY (owner_id) REFERENCES roles(role_name)
);

CREATE TABLE IF NOT EXISTS links_role_item_history (
    history_event_id BIGSERIAL PRIMARY KEY,
    history_user_id text NOT NULL,
    history_originator_id text,
    history_action text NOT NULL,
    history_timestamp integer NOT NULL,
    history_comment text,

    link_id INTEGER NOT NULL,
    slave_id TEXT NOT NULL,
    owner_id TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS links_role_role (
    link_id SERIAL NOT NULL PRIMARY KEY,
    slave_id TEXT NOT NULL,
    owner_id TEXT NOT NULL,
    FOREIGN KEY (slave_id) REFERENCES roles(role_name),
    FOREIGN KEY (owner_id) REFERENCES roles(role_name)
);

CREATE TABLE IF NOT EXISTS links_role_role_history (
    history_event_id BIGSERIAL PRIMARY KEY,
    history_user_id text NOT NULL,
    history_originator_id text,
    history_action text NOT NULL,
    history_timestamp integer NOT NULL,
    history_comment text,

    link_id INTEGER NOT NULL,
    slave_id TEXT NOT NULL,
    owner_id TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS links_user_role (
    link_id SERIAL NOT NULL PRIMARY KEY,
    owner_id TEXT NOT NULL,
    slave_id INTEGER NOT NULL,
    FOREIGN KEY (slave_id) REFERENCES roles(role_id)
);

CREATE TABLE IF NOT EXISTS links_user_role_history (
    history_event_id BIGSERIAL PRIMARY KEY,
    history_user_id text NOT NULL,
    history_originator_id text,
    history_action text NOT NULL,
    history_timestamp integer NOT NULL,
    history_comment text,

    link_id INTEGER NOT NULL,
    slave_id TEXT NOT NULL,
    owner_id TEXT NOT NULL
);


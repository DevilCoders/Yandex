CREATE TABLE IF NOT EXISTS cs_localization (
    resource_id text NOT NULL PRIMARY KEY,
    revision BIGSERIAL NOT NULL,
    resource_data json NOT NULL
);

CREATE TABLE IF NOT EXISTS cs_localization_history (
    history_event_id BIGSERIAL PRIMARY KEY,
    history_user_id text NOT NULL,
    history_originator_id text,
    history_action text NOT NULL,
    history_timestamp integer NOT NULL,
    history_comment text,
 
    resource_id text NOT NULL,
    revision BIGSERIAL NOT NULL,
    resource_data json NOT NULL
); 
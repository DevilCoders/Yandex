CREATE TABLE IF NOT EXISTS cs_propositions (
    proposition_id SERIAL PRIMARY KEY,
    proposition_object_id TEXT,
    proposition_category_id TEXT,
    revision SERIAL NOT NULL UNIQUE,
    class_name TEXT NOT NULL,
    data TEXT NOT NULL
);


CREATE TABLE IF NOT EXISTS cs_propositions_history (
    history_event_id BIGSERIAL PRIMARY KEY,
    history_user_id text NOT NULL,
    history_originator_id text,
    history_action text NOT NULL,
    history_timestamp integer NOT NULL,
    history_comment text,

    proposition_id SERIAL NOT NULL,
    proposition_object_id TEXT,
    proposition_category_id TEXT,
    revision SERIAL NOT NULL,
    class_name TEXT NOT NULL,
    data TEXT NOT NULL
);
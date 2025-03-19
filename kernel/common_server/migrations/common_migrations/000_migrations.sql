CREATE TABLE IF NOT EXISTS migrations_history (
    filename text NOT NULL PRIMARY KEY,
    source text,
    apply_ts integer,
    applied_hash text,
    revision SERIAL
);

DO $$ BEGIN
    IF NOT EXISTS (SELECT constraint_name FROM information_schema.table_constraints WHERE table_name = 'migrations_history' AND constraint_type = 'PRIMARY KEY') THEN
        ALTER TABLE migrations_history
          ADD PRIMARY KEY (filename);
    END IF;
END $$;

ALTER TABLE migrations_history ADD COLUMN IF NOT EXISTS source text;
ALTER TABLE migrations_history ADD COLUMN IF NOT EXISTS applied_hash text;
ALTER TABLE migrations_history ADD COLUMN IF NOT EXISTS revision SERIAL;

CREATE TABLE IF NOT EXISTS migrations_history_history (
    history_event_id BIGSERIAL PRIMARY KEY,
    history_user_id text NOT NULL,
    history_originator_id text,
    history_action text NOT NULL,
    history_timestamp integer NOT NULL,
    history_comment text,

    filename text NOT NULL,
    source text,
    apply_ts integer,
    applied_hash text,
    revision integer
);

DO $$ BEGIN
    IF NOT EXISTS (SELECT constraint_name FROM information_schema.table_constraints WHERE table_name = 'migrations_history_history' AND constraint_type = 'PRIMARY KEY') THEN
        ALTER TABLE migrations_history_history
          ADD PRIMARY KEY (filename);
    END IF;
END $$;

ALTER TABLE migrations_history_history ADD COLUMN IF NOT EXISTS source text;
ALTER TABLE migrations_history_history ADD COLUMN IF NOT EXISTS applied_hash text;
ALTER TABLE migrations_history_history ADD COLUMN IF NOT EXISTS revision SERIAL;

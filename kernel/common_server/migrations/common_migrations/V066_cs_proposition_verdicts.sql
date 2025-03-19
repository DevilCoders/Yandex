CREATE TABLE IF NOT EXISTS cs_proposition_verdicts (
    verdict_id SERIAL PRIMARY KEY,
    proposition_id INTEGER NOT NULL,
    proposition_revision INTEGER NOT NULL,
    comment TEXT,
    system_user_id TEXT NOT NULL,
    verdict TEXT NOT NULL,
    verdict_instant INTEGER NOT NULL
);


DO $$ BEGIN
    IF NOT EXISTS (SELECT constraint_name FROM information_schema.table_constraints WHERE table_name = 'cs_proposition_verdicts' AND constraint_name = 'cs_proposition_verdicts_cs_propositions_proposition_id_fk') THEN
        ALTER TABLE cs_proposition_verdicts ADD CONSTRAINT cs_proposition_verdicts_cs_propositions_proposition_id_fk
        FOREIGN KEY (proposition_id) REFERENCES cs_propositions(proposition_id);
    END IF;
END $$;


CREATE TABLE IF NOT EXISTS cs_proposition_verdicts_history (
    history_event_id BIGSERIAL PRIMARY KEY,
    history_user_id text NOT NULL,
    history_originator_id text,
    history_action text NOT NULL,
    history_timestamp integer NOT NULL,
    history_comment text,

    verdict_id SERIAL NOT NULL,
    proposition_id INTEGER NOT NULL,
    proposition_revision INTEGER NOT NULL,
    comment TEXT,
    system_user_id TEXT NOT NULL,
    verdict TEXT NOT NULL,
    verdict_instant INTEGER NOT NULL
);
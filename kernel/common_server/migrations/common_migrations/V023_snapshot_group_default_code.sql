ALTER TABLE snc_groups ADD COLUMN IF NOT EXISTS default_snapshot_code TEXT;
ALTER TABLE snc_groups_history ADD COLUMN IF NOT EXISTS default_snapshot_code TEXT;

DO $$ BEGIN
    IF NOT EXISTS (SELECT constraint_name FROM information_schema.table_constraints WHERE table_name = 'snc_groups' AND constraint_name = 'snc_groups_snapshot_code_fk') THEN
        ALTER TABLE snc_groups ADD CONSTRAINT snc_groups_snapshot_code_fk FOREIGN KEY (default_snapshot_code) REFERENCES snc_snapshots(snapshot_code);
    END IF;
END $$;

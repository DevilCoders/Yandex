DO $$ BEGIN
    IF NOT EXISTS (SELECT constraint_name FROM information_schema.table_constraints WHERE constraint_name = 'links_user_role_owner_id_slave_id_uq') THEN
        ALTER TABLE links_user_role ADD CONSTRAINT links_user_role_owner_id_slave_id_uq UNIQUE (owner_id, slave_id);
    END IF;
END $$;

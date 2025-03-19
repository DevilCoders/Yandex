DO $$ BEGIN
    IF NOT EXISTS (SELECT constraint_name FROM information_schema.table_constraints WHERE constraint_name = 'links_role_role_owner_id_slave_id_uq') THEN
        ALTER TABLE links_role_role ADD CONSTRAINT links_role_role_owner_id_slave_id_uq UNIQUE (owner_id, slave_id); 
    END IF;
END $$;

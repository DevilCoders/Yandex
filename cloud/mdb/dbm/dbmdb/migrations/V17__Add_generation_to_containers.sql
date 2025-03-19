ALTER TABLE mdb.containers
    ADD COLUMN generation integer NOT NULL DEFAULT 1;

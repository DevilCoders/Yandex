ALTER TABLE mdb.containers
    ADD COLUMN project_id text;

ALTER TABLE mdb.containers
    ADD COLUMN managing_project_id text;

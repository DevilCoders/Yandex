ALTER TABLE mdb.containers ADD COLUMN secrets jsonb;

ALTER TABLE mdb.containers ADD COLUMN secrets_expire timestamp WITH TIME ZONE;

CREATE INDEX i_containers_secrets_expire ON mdb.containers USING btree (
    secrets_expire )
WHERE
    secrets_expire IS NOT NULL;

ALTER TABLE mdb.containers ADD CONSTRAINT check_secrets_expire CHECK (
    (
        secrets_expire IS NOT NULL
        AND secrets IS NOT NULL )
    OR (
        secrets_expire IS NULL
        AND secrets IS NULL ) );

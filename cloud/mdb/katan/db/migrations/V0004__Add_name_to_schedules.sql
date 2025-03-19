COMMENT ON COLUMN katan.schedules.namespace IS NULL;

ALTER TABLE katan.schedules ADD COLUMN name text;

UPDATE katan.schedules SET name = 'highstate';

ALTER TABLE katan.schedules ADD CONSTRAINT check_name_not_empty CHECK (
        length(name) > 0
    );

ALTER TABLE katan.schedules ALTER COLUMN name SET NOT NULL;


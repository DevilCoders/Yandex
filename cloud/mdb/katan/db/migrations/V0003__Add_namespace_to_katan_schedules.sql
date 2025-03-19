ALTER TABLE katan.schedules ADD COLUMN namespace text;

UPDATE katan.schedules SET namespace = 'core';

ALTER TABLE katan.schedules ALTER COLUMN namespace SET NOT NULL;
ALTER TABLE katan.schedules ADD CONSTRAINT check_namespace_not_empty CHECK (
        length(namespace) > 0
);

COMMENT ON COLUMN katan.schedules.namespace IS 'juggler namespace';

ALTER TABLE katan.schedules ADD COLUMN examined_rollout_id bigint;
ALTER TABLE katan.schedules ADD CONSTRAINT check_examined_rollout_id_is_not_zero CHECK (
        examined_rollout_id != 0
);

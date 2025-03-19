CREATE TABLE katan.schedule_fails (
    schedule_id  bigint NOT NULL,
    rollout_id   bigint NOT NULL,
    cluster_id   text   NOT NULL,

    CONSTRAINT pk_schedule_fails PRIMARY KEY (schedule_id, rollout_id, cluster_id),
    CONSTRAINT fk_schedule_fails_to_schedules FOREIGN KEY (schedule_id)
        REFERENCES katan.schedules ON DELETE CASCADE,
    CONSTRAINT fk_schedule_fails_to_cluster_rollouts FOREIGN KEY (rollout_id, cluster_id)
        REFERENCES katan.cluster_rollouts ON DELETE CASCADE
);


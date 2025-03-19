CREATE SCHEMA katan;

CREATE TABLE katan.clusters (
    cluster_id   text NOT NULL,
    tags         jsonb NOT NULL,
    imported_at  timestamptz NOT NULL DEFAULT now(),
    auto_update  boolean NOT NULL DEFAULT true,
    CONSTRAINT check_tags_contain_source_and_be_an_object CHECK (
        tags ? 'source'
    ),

    CONSTRAINT pk_clusters PRIMARY KEY (cluster_id)
);

CREATE INDEX i_clusters_tags
    ON katan.clusters USING gin (tags) WITH (fastupdate = OFF);

CREATE TABLE katan.hosts (
    fqdn         text  NOT NULL,
    cluster_id   text  NOT NULL,
    tags         jsonb NOT NULL,

    CONSTRAINT pk_hosts PRIMARY KEY (fqdn),
    CONSTRAINT fk_hosts_to_clusters
       FOREIGN KEY (cluster_id) REFERENCES katan.clusters
       ON DELETE CASCADE
);

CREATE INDEX i_host_cluster_id
    ON katan.hosts (cluster_id);
CREATE INDEX i_host_tags
    ON katan.hosts USING gin (tags) WITH (fastupdate = OFF);


CREATE TYPE katan.schedule_state AS ENUM (
    'active',
    'stopped',
    'broken'
);

CREATE TABLE katan.schedules (
    schedule_id bigint GENERATED ALWAYS AS IDENTITY NOT NULL,
    match_tags  jsonb NOT NULL,
    commands    jsonb NOT NULL,
    state       katan.schedule_state NOT NULL DEFAULT 'active',
    age         interval NOT NULL,
    still_age   interval NOT NULL,
    max_size    bigint NOT NULL,
    parallel    integer NOT NULL,
    edited_at   timestamptz NOT NULL DEFAULT now(),
    edited_by   text,

    CONSTRAINT pk_schedules PRIMARY KEY (schedule_id),
    CONSTRAINT check_match_tags_should_be_an_object CHECK (
        jsonb_typeof(match_tags) = 'object'
    ),
    CONSTRAINT check_age_should_be_positive CHECK ( age > '0s' ),
    CONSTRAINT check_still_age_should_be_positive CHECK ( still_age > '0s' ),
    CONSTRAINT check_max_size_should_be_positive CHECK (
        max_size > 0
    ),
    CONSTRAINT check_parallel_should_be_positive CHECK (
        parallel > 0
    ),
    CONSTRAINT check_still_age_less_then_age CHECK (
        still_age < age
    )
);


CREATE TABLE katan.schedule_dependencies (
    schedule_id   bigint NOT NULL,
    depends_on_id bigint NOT NULL,

    CONSTRAINT pk_schedule_dependencies PRIMARY KEY (schedule_id, depends_on_id),
    CONSTRAINT fk_schedule_dependencies_schedule_id FOREIGN KEY (schedule_id)
        REFERENCES katan.schedules (schedule_id) ON DELETE CASCADE,
    CONSTRAINT fk_schedule_dependencies_depends_on_id FOREIGN KEY (depends_on_id)
        REFERENCES katan.schedules (schedule_id) ON DELETE CASCADE
);

CREATE INDEX i_schedule_dependencies_depends_on_id ON katan.schedule_dependencies (depends_on_id);


CREATE TABLE katan.rollouts (
    rollout_id   bigint GENERATED ALWAYS AS IDENTITY NOT NULL,
    commands     jsonb NOT NULL,
    parallel     integer NOT NULL,
    created_at   timestamptz NOT NULL DEFAULT now(),
    started_at   timestamptz,
    finished_at  timestamptz,
    created_by   text,
    schedule_id  bigint,
    comment      text,

    CONSTRAINT pk_rollouts PRIMARY KEY (rollout_id),
    CONSTRAINT fk_rollouts_schedules FOREIGN KEY (schedule_id)
        REFERENCES katan.schedules (schedule_id),
    CONSTRAINT check_not_started_rollout_can_not_be_finished CHECK (
        finished_at IS NULL OR started_at IS NOT NULL
    ),
    CONSTRAINT check_parallel_should_be_positive CHECK (
            parallel > 0
    )
);

CREATE INDEX i_rollouts_to_schedules ON katan.rollouts (schedule_id);

CREATE TABLE katan.rollouts_dependencies (
    rollout_id    bigint NOT NULL,
    depends_on_id bigint NOT NULL,
    CONSTRAINT pk_rollouts_dependencies PRIMARY KEY (rollout_id, depends_on_id),
    CONSTRAINT fk_rollouts_dependencies_rollout_id FOREIGN KEY (rollout_id)
        REFERENCES katan.rollouts (rollout_id) ON DELETE CASCADE,
    CONSTRAINT fk_rollouts_dependencies_depends_on_id FOREIGN KEY (depends_on_id)
        REFERENCES katan.rollouts (rollout_id) ON DELETE CASCADE
);
CREATE INDEX i_rollouts_dependencies_depends_on_id ON katan.rollouts_dependencies (depends_on_id);


CREATE TYPE katan.cluster_rollout_state AS ENUM (
    'pending',
    'running',
    'succeeded',
    'cancelled',
    'skipped',
    'failed'
);

CREATE TABLE katan.cluster_rollouts (
    rollout_id  bigint NOT NULL,
    cluster_id  text   NOT NULL,
    state       katan.cluster_rollout_state NOT NULL DEFAULT 'pending',
    updated_at  timestamptz NOT NULL DEFAULT now(),
    comment     text,

    CONSTRAINT pk_cluster_rollouts PRIMARY KEY (rollout_id, cluster_id),
    CONSTRAINT fk_cluster_rollouts_to_clusters FOREIGN KEY (cluster_id)
        REFERENCES katan.clusters
        ON DELETE CASCADE,
    CONSTRAINT fk_cluster_rollouts_to_rollouts FOREIGN KEY (rollout_id)
        REFERENCES katan.rollouts
        ON DELETE CASCADE
);

CREATE INDEX i_cluster_rollouts_cluster_id ON katan.cluster_rollouts (cluster_id);

CREATE UNIQUE INDEX uk_cluster_rollouts_cluster_id_where_running ON katan.cluster_rollouts (cluster_id)
    WHERE state = 'running';

COMMENT ON INDEX katan.uk_cluster_rollouts_cluster_id_where_running
    IS 'Should be only one running rollout for the cluster';

CREATE TABLE katan.rollout_shipments (
    rollout_id  bigint NOT NULL,
    fqdn        text   NOT NULL,
    shipment_id bigint NOT NULL,

    CONSTRAINT pk_rollout_shipments PRIMARY KEY (rollout_id, fqdn, shipment_id),
    CONSTRAINT fk_rollout_shipments_to_rollouts FOREIGN KEY (rollout_id)
        REFERENCES katan.rollouts ON DELETE CASCADE
);


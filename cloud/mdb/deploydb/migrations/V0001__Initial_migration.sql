CREATE SCHEMA deploy;

CREATE TABLE deploy.groups (
    group_id integer   NOT NULL GENERATED ALWAYS AS IDENTITY,
    name     text      NOT NULL,

    CONSTRAINT pk_groups PRIMARY KEY (group_id),
    CONSTRAINT uk_groups_name UNIQUE (name),

    CONSTRAINT check_name_length CHECK (
        char_length(name) BETWEEN 1 AND 1024
    )
);


CREATE TABLE deploy.masters (
    master_id         bigint       NOT NULL GENERATED ALWAYS AS IDENTITY,
    group_id          integer      NOT NULL,
    fqdn              text         NOT NULL,
    is_open           boolean      NOT NULL,
    description       text,
    created_at        timestamptz  NOT NULL DEFAULT now(),
    updated_at        timestamptz  NOT NULL DEFAULT now(),
    alive_check_at    timestamptz  NOT NULL DEFAULT now(),
    is_alive          boolean      NOT NULL,

    CONSTRAINT pk_masters PRIMARY KEY (master_id),
    CONSTRAINT uk_masters_fqdn UNIQUE (fqdn),
    CONSTRAINT uk_masters_master_id_group_id UNIQUE (master_id, group_id),

    CONSTRAINT fk_masters_groups FOREIGN KEY (group_id)
        REFERENCES deploy.groups ON DELETE NO ACTION,

    CONSTRAINT check_fqdn_length CHECK (
        char_length(fqdn) BETWEEN 1 AND 512
    )
);


CREATE TABLE deploy.master_aliases (
    master_id     bigint     NOT NULL,
    alias         text       NOT NULL,

    CONSTRAINT pk_master_aliases PRIMARY KEY (master_id),
    CONSTRAINT uk_master_aliases_alias UNIQUE (alias),

    CONSTRAINT fk_master_aliases_master_id FOREIGN KEY (master_id)
        REFERENCES deploy.masters (master_id) ON DELETE CASCADE,

    CONSTRAINT check_alias_length CHECK (
        char_length(alias) BETWEEN 1 AND 512
    )
);


CREATE TYPE deploy.master_change_type AS ENUM (
    'create',
    'delete',
    'update',
    'add-alias',
    'remove-alias'
);

CREATE TABLE deploy.masters_change_log (
    change_id   bigint                    NOT NULL GENERATED ALWAYS AS IDENTITY,
    master_id   bigint                    NOT NULL,
    changed_at  timestamptz               NOT NULL DEFAULT now(),
    change_type deploy.master_change_type NOT NULL,
    old_row     jsonb,
    new_row     jsonb,

    CONSTRAINT pk_masters_change_log PRIMARY KEY (change_id)
);

CREATE INDEX i_masters_change_log_master_id
    ON deploy.masters_change_log (master_id, changed_at);


CREATE TABLE deploy.minions (
    minion_id        bigint      NOT NULL GENERATED ALWAYS AS IDENTITY,
    fqdn             text        NOT NULL,
    group_id         integer     NOT NULL,
    master_id        bigint      NOT NULL,
    pub_key          text,
    auto_reassign    boolean     NOT NULL,
    created_at       timestamptz NOT NULL DEFAULT now(),
    updated_at       timestamptz NOT NULL DEFAULT now(),
    register_until   timestamptz,

    CONSTRAINT pk_minions PRIMARY KEY (minion_id),
    CONSTRAINT uk_minions_fqdn UNIQUE (fqdn),

    CONSTRAINT fk_minions_master_id FOREIGN KEY (master_id, group_id)
        REFERENCES deploy.masters (master_id, group_id) ON DELETE NO ACTION,

    CONSTRAINT check_fqdn_length CHECK (
        char_length(fqdn) BETWEEN 1 AND 512
    ),
    CONSTRAINT check_registered CHECK (
        -- registered
        (register_until IS NOT NULL AND pub_key IS NULL)
        OR
        -- unregisted
        (register_until IS NULL AND pub_key IS NOT NULL)
    )
);

CREATE INDEX i_minions_master_id ON deploy.minions (master_id);

CREATE TYPE deploy.minion_change_type AS ENUM (
    'create',
    'delete',
    'register',
    'reassign'
);

CREATE TABLE deploy.minions_change_log (
    change_id   bigint                     NOT NULL GENERATED ALWAYS AS IDENTITY,
    minion_id   bigint                     NOT NULL,
    changed_at  timestamptz                NOT NULL DEFAULT now(),
    change_type deploy.minion_change_type  NOT NULL,
    old_row     jsonb,
    new_row     jsonb,

    CONSTRAINT pk_minions_change_log PRIMARY KEY (change_id)
);

CREATE INDEX i_minions_change_log_minion_id
    ON deploy.minions_change_log (minion_id, changed_at);

CREATE TYPE deploy.shipment_status AS ENUM (
    'INPROGRESS',
    'DONE',
    'ERROR'
);

CREATE TABLE deploy.shipments (
    shipment_id             bigint                  NOT NULL GENERATED ALWAYS AS IDENTITY,
    type                    text                    NOT NULL,
    fqdns                   text[]                  NOT NULL,
    status                  deploy.shipment_status  NOT NULL,
    batch_size              bigint                  NOT NULL,
    stop_on_error_count     bigint                  NOT NULL,
    other_count             bigint                  NOT NULL,
    done_count              bigint                  NOT NULL,
    errors_count            bigint                  NOT NULL,
    total_count             bigint                  NOT NULL,
    created_at              timestamptz             NOT NULL DEFAULT now(),
    updated_at              timestamptz             NOT NULL DEFAULT now(),

    CONSTRAINT pk_shipments PRIMARY KEY (shipment_id),

    CONSTRAINT check_total_count CHECK (
        total_count > 0
    ),

    CONSTRAINT check_count CHECK (
        other_count + done_count + errors_count = total_count
    ),

    CONSTRAINT check_type_length CHECK (
        char_length(type) BETWEEN 1 AND 512
    )
);

CREATE TYPE deploy.command_status AS ENUM (
    'BLOCKED',
    'AVAILABLE',
    'RUNNING',
    'DONE',
    'ERROR'
);

CREATE TABLE deploy.commands (
    command_id        bigint                NOT NULL GENERATED ALWAYS AS IDENTITY,
    shipment_id       bigint                NOT NULL,
    type              text                  NOT NULL,
    minion_id         bigint                NOT NULL,
    status            deploy.command_status NOT NULL,
    retries           bigint                NOT NULL DEFAULT 0,
    created_at        timestamptz           NOT NULL DEFAULT now(),
    updated_at        timestamptz           NOT NULL DEFAULT now(),

    CONSTRAINT pk_commands PRIMARY KEY (command_id),

    CONSTRAINT fk_commands_shipment_id FOREIGN KEY (shipment_id)
        REFERENCES deploy.shipments (shipment_id) ON DELETE CASCADE,

    CONSTRAINT fk_commands_minion_id FOREIGN KEY (minion_id)
        REFERENCES deploy.minions (minion_id) ON DELETE CASCADE,

    CONSTRAINT check_type_length CHECK (
        char_length(type) BETWEEN 1 AND 512
    )
);

CREATE INDEX i_commands_shipment_id
    ON deploy.commands (shipment_id);

CREATE INDEX i_commands_minion_id_created_at
    ON deploy.commands (minion_id, created_at);

CREATE TYPE deploy.job_status AS ENUM (
    'RUNNING',
    'DONE',
    'ERROR'
);

CREATE TABLE deploy.jobs (
    job_id            bigint              NOT NULL GENERATED ALWAYS AS IDENTITY,
    ext_job_id        text                NOT NULL,
    command_id        bigint              NOT NULL,
    status            deploy.job_status   NOT NULL,
    created_at        timestamptz         NOT NULL DEFAULT now(),
    updated_at        timestamptz         NOT NULL DEFAULT now(),

    CONSTRAINT pk_jobs PRIMARY KEY (job_id),

    CONSTRAINT fk_jobs_command_id FOREIGN KEY (command_id)
        REFERENCES deploy.commands (command_id) ON DELETE CASCADE,

    CONSTRAINT check_ext_job_id_length CHECK (
        char_length(ext_job_id) BETWEEN 1 AND 512
    )
);

CREATE INDEX i_jobs_command_id
    ON deploy.jobs (command_id);

CREATE TYPE deploy.job_result_status AS ENUM (
    'SUCCESS',
    'FAILURE',
    'UNKNOWN'
);

CREATE TABLE deploy.job_results (
    ext_job_id   text                       NOT NULL,
    fqdn         text                       NOT NULL,
    status       deploy.job_result_status   NOT NULL,
    result       jsonb,
    recorded_at  timestamptz                NOT NULL DEFAULT now(),

    CONSTRAINT pk_job_results PRIMARY KEY (ext_job_id, fqdn),

    CONSTRAINT check_ext_job_id_length CHECK (
        char_length(ext_job_id) BETWEEN 1 AND 512
    ),
    CONSTRAINT check_fqdn_length CHECK (
        char_length(fqdn) BETWEEN 1 AND 512
    )
);

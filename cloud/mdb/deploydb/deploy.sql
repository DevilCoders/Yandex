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

CREATE TABLE deploy.masters_check_view (
    master_id         bigint       NOT NULL,
    checker_fqdn      text         NOT NULL,
    is_alive          boolean      NOT NULL,
    updated_at        timestamptz  NOT NULL,

    CONSTRAINT pk_masters_check_view PRIMARY KEY (master_id, checker_fqdn),

    CONSTRAINT fk_masters_check_view_master_id FOREIGN KEY (master_id)
        REFERENCES deploy.masters ON DELETE CASCADE,

    CONSTRAINT check_fqdn_length CHECK (
        char_length(checker_fqdn) BETWEEN 1 AND 512
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
    deleted          boolean     NOT NULL DEFAULT false,

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
        OR
        -- allowed to reregister
        (register_until IS NOT NULL AND pub_key IS NOT NULL)
    )
);

CREATE INDEX i_minions_master_id ON deploy.minions (master_id);
CREATE INDEX i_minions_deleted_updated_at_minion_id_fqdn
    ON deploy.minions (updated_at, minion_id, fqdn) WHERE deleted;


CREATE TYPE deploy.minion_change_type AS ENUM (
    'create',
    'delete',
    'register',
    'reassign',
    'unregister'
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
    'ERROR',
    'TIMEOUT'
);

CREATE TABLE deploy.shipments (
    shipment_id             bigint                  NOT NULL GENERATED ALWAYS AS IDENTITY,
    fqdns                   text[]                  NOT NULL,
    status                  deploy.shipment_status  NOT NULL,
    parallel                bigint                  NOT NULL,
    stop_on_error_count     bigint                  NOT NULL,
    other_count             bigint                  NOT NULL,
    done_count              bigint                  NOT NULL,
    errors_count            bigint                  NOT NULL,
    total_count             bigint                  NOT NULL,
    created_at              timestamptz             NOT NULL,
    updated_at              timestamptz             NOT NULL,
    timeout                 interval                NOT NULL,
    tracing                 text,

    CONSTRAINT pk_shipments PRIMARY KEY (shipment_id),

    CONSTRAINT check_parallel CHECK (
        parallel > 0
    ),

    CONSTRAINT check_stop_on_error_count CHECK (
        stop_on_error_count >= 0 AND stop_on_error_count <= total_count
    ),

    CONSTRAINT check_total_count CHECK (
        total_count > 0
    ),

    CONSTRAINT check_count CHECK (
        other_count + done_count + errors_count = total_count
    ),

    CONSTRAINT check_fqdns CHECK (
        array_length(fqdns, 1) > 0
    )
);

CREATE INDEX i_shipments_created_at_timeout
    ON deploy.shipments (created_at, timeout)
 WHERE STATUS = 'INPROGRESS'::deploy.shipment_status;


CREATE TABLE deploy.shipment_commands (
    shipment_command_id  bigint NOT NULL GENERATED ALWAYS AS IDENTITY,
    shipment_id          bigint NOT NULL,
    type                 text   NOT NULL,
    arguments            text[],
    timeout              interval,

    CONSTRAINT pk_shipment_commands PRIMARY KEY (shipment_command_id),
    CONSTRAINT check_type_length CHECK (
        char_length(type) BETWEEN 1 AND 512
    )
);

CREATE INDEX i_shipment_commands_shipment_id
    ON deploy.shipment_commands (shipment_id);


CREATE TYPE deploy.command_status AS ENUM (
    'BLOCKED',
    'AVAILABLE',
    'RUNNING',
    'DONE',
    'ERROR',
    'CANCELED',
    'TIMEOUT'
);

CREATE TABLE deploy.commands (
    command_id                  bigint                NOT NULL GENERATED ALWAYS AS IDENTITY,
    minion_id                   bigint                NOT NULL,
    status                      deploy.command_status NOT NULL,
    created_at                  timestamptz           NOT NULL,
    updated_at                  timestamptz           NOT NULL,
    shipment_command_id         bigint                NOT NULL,
    last_dispatch_attempt_at    timestamptz,

    CONSTRAINT pk_commands PRIMARY KEY (command_id),

    CONSTRAINT fk_commands_shipment_command_id FOREIGN KEY (shipment_command_id)
        REFERENCES deploy.shipment_commands (shipment_command_id) ON DELETE CASCADE,

    CONSTRAINT fk_commands_minion_id FOREIGN KEY (minion_id)
        REFERENCES deploy.minions (minion_id) ON DELETE CASCADE
);

CREATE INDEX i_commands_shipment_command_id
    ON deploy.commands (shipment_command_id);

CREATE INDEX i_commands_minion_id_created_at
    ON deploy.commands (minion_id, created_at);

CREATE INDEX i_commands_minion_id_last_dispatch_attempt_at
    ON deploy.commands (minion_id, last_dispatch_attempt_at)
 WHERE status = 'AVAILABLE'::deploy.command_status;

CREATE INDEX i_commands_minion_id_status_running
    ON deploy.commands (minion_id)
    WHERE status = 'RUNNING'::deploy.command_status;


CREATE TYPE deploy.job_status AS ENUM (
    'RUNNING',
    'DONE',
    'ERROR',
    'TIMEOUT'
);

CREATE TABLE deploy.jobs (
    job_id                  bigint              NOT NULL GENERATED ALWAYS AS IDENTITY,
    ext_job_id              text                NOT NULL,
    command_id              bigint              NOT NULL,
    status                  deploy.job_status   NOT NULL,
    created_at              timestamptz         NOT NULL,
    updated_at              timestamptz         NOT NULL,
    last_running_check_at   timestamptz         NOT NULL,
    running_checks_failed   int                 NOT NULL DEFAULT 0,

    CONSTRAINT pk_jobs PRIMARY KEY (job_id),

    CONSTRAINT fk_jobs_command_id FOREIGN KEY (command_id)
        REFERENCES deploy.commands (command_id) ON DELETE CASCADE,

    CONSTRAINT check_ext_job_id_length CHECK (
        char_length(ext_job_id) BETWEEN 1 AND 512
    )
);

CREATE INDEX i_jobs_command_id
    ON deploy.jobs (command_id);

CREATE INDEX i_jobs_status
    ON deploy.jobs (status);

CREATE INDEX i_jobs_ext_job_id
    ON deploy.jobs (ext_job_id);

CREATE TYPE deploy.job_result_status AS ENUM (
    'SUCCESS',
    'FAILURE',
    'UNKNOWN',
    'TIMEOUT',
    'NOTRUNNING'
);

CREATE TABLE deploy.job_results (
    ext_job_id      text                        NOT NULL,
    fqdn            text                        NOT NULL,
    status          deploy.job_result_status,
    result          jsonb,
    recorded_at     timestamptz                 NOT NULL,
    order_id        integer                     NOT NULL,
    job_result_id   bigint                      NOT NULL GENERATED ALWAYS AS IDENTITY,

    CONSTRAINT pk_job_results PRIMARY KEY (ext_job_id, fqdn, order_id),

    CONSTRAINT check_ext_job_id_length CHECK (
        char_length(ext_job_id) BETWEEN 1 AND 512
    ),
    CONSTRAINT check_fqdn_length CHECK (
        char_length(fqdn) BETWEEN 1 AND 512
    )
);

CREATE INDEX i_job_results_job_result_id
    ON deploy.job_results (job_result_id);
CREATE INDEX i_job_results_fqdn
    ON deploy.job_results USING HASH(fqdn);

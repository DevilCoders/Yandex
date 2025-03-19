CREATE TYPE code.deploy_group AS (
    id      integer,
    name    text
);

CREATE TYPE code.master AS (
    id             bigint,
    fqdn           text,
    aliases        text[],
    deploy_group   text,
    is_open        boolean,
    description    text,
    created_at     timestamptz,
    alive_check_at timestamptz,
    is_alive       boolean
);

CREATE TYPE code.minion AS (
    id             bigint,
    fqdn           text,
    deploy_group   text,
    master_fqdn    text,
    auto_reassign  boolean,
    created_at     timestamptz,
    updated_at     timestamptz,
    register_until timestamptz,
    pub_key        text,
    registered     boolean,
    deleted        boolean
);

CREATE TYPE code.command_def AS (
    type      text,
    arguments text[],
    timeout   interval
);

CREATE TYPE code.shipment AS (
    shipment_id           bigint,
    commands              json,
    fqdns                 text[],
    status                text, -- deploy.shipment_status
    parallel              bigint,
    stop_on_error_count   bigint,
    other_count           bigint,
    done_count            bigint,
    errors_count          bigint,
    total_count           bigint,
    created_at            timestamptz,
    updated_at            timestamptz,
    timeout               interval,
    tracing               text
);

CREATE TYPE code.command AS (
    id             bigint,
    shipment_id    bigint,
    type           text,
    arguments      text[],
    fqdn           text,
    status         text, -- deploy.command_status
    created_at     timestamptz,
    updated_at     timestamptz,
    timeout        interval
);

CREATE TYPE code.job AS (
    id             bigint,
    ext_id         text,
    command_id     bigint,
    status         text, -- deploy.job_status
    created_at     timestamptz,
    updated_at     timestamptz
);

CREATE TYPE code.job_result AS (
    id             bigint,
    order_id       integer,
    ext_id         text,
    fqdn           text,
    status         text, -- deploy.job_result_status
    result         jsonb,
    recorded_at    timestamptz
);

CREATE TYPE code.shipment_script AS (
    shipment_id               bigint,
    first_shipment_command_id bigint,
    next_shipment_command_id  bigint,
    shipment_command_ids      bigint[]
);

CREATE TYPE code.job_result_coords AS (
    shipment_id     bigint,
    command_id      bigint,
    job_id          bigint,
    tracing         text
);

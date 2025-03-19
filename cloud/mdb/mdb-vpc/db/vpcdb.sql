CREATE SCHEMA vpc;

CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TYPE vpc.network_status AS ENUM (
    'CREATING',
    'ACTIVE',
    'DELETING',
    'ERROR'
    );

CREATE TABLE vpc.networks
(
    network_id         UUID PRIMARY KEY         DEFAULT gen_random_uuid(),
    project_id         text                                         NOT NULL,
    cloud_provider     text                                         NOT NULL,
    region_id          text                                         NOT NULL,
    create_time        timestamp with time zone DEFAULT now()       NOT NULL,
    name               text                                         NOT NULL,
    description        text,
    ipv4_cidr_block    cidr                                         NOT NULL,
    ipv6_cidr_block    cidr                     DEFAULT '::'::cidr  NOT NULL,
    status             vpc.network_status       DEFAULT 'CREATING'  NOT NULL,
    status_reason      text,
    external_resources jsonb                    DEFAULT '{}'::jsonb NOT NULL,
    CONSTRAINT uniq_name_per_project UNIQUE (project_id, name)
);

CREATE INDEX networks_project ON vpc.networks (project_id);
CREATE UNIQUE INDEX networks_aws_vpc_id on vpc.networks (NULLIF(external_resources ->> 'vpc_id', ''), region_id);

CREATE TYPE vpc.operation_status AS ENUM (
    'PENDING',
    'RUNNING',
    'DONE'
    );

CREATE TABLE vpc.operations
(
    operation_id   UUID PRIMARY KEY         DEFAULT gen_random_uuid(),
    project_id     text                                         NOT NULL,
    description    text,
    created_by     text                                         NOT NULL,
    metadata       jsonb,
    create_time    timestamp with time zone DEFAULT now()       NOT NULL,
    start_time     timestamp with time zone,
    finish_time    timestamp with time zone,
    status         vpc.operation_status     DEFAULT 'PENDING'   NOT NULL,
    state          jsonb                    DEFAULT '{}'::jsonb NOT NULL,
    action         text                                         NOT NULL,
    cloud_provider text                                         NOT NULL,
    region         text                                         NOT NULL
);

CREATE INDEX operation_status ON vpc.operations (status);

CREATE TYPE vpc.network_connection_status AS ENUM (
    'CREATING',
    'PENDING',
    'ACTIVE',
    'DELETING',
    'ERROR'
    );

CREATE TABLE vpc.network_connections
(
    network_connection_id UUID PRIMARY KEY              DEFAULT gen_random_uuid(),
    network_id            UUID REFERENCES vpc.networks ON DELETE RESTRICT,
    project_id            text                                              NOT NULL,
    cloud_provider        text                                              NOT NULL,
    region_id             text                                              NOT NULL,
    create_time           timestamp with time zone      DEFAULT now()       NOT NULL,
    description           text,
    status                vpc.network_connection_status DEFAULT 'CREATING'  NOT NULL,
    status_reason         text,
    connection_params     jsonb                         DEFAULT '{}'::jsonb NOT NULL
);

CREATE INDEX network_connections_project ON vpc.network_connections (project_id);
CREATE INDEX network_connections_network ON vpc.network_connections (network_id);

GRANT USAGE ON SCHEMA vpc TO vpc_worker;
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA vpc TO vpc_worker;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA vpc TO vpc_worker;
GRANT SELECT ON TABLE public.schema_version TO vpc_worker;

GRANT USAGE ON SCHEMA vpc TO vpc_api;
GRANT SELECT, INSERT, UPDATE ON vpc.networks TO vpc_api;
GRANT SELECT, INSERT, UPDATE ON vpc.network_connections TO vpc_api;
GRANT SELECT, INSERT ON vpc.operations TO vpc_api;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA vpc TO vpc_api;

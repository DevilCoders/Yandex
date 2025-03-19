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
    network_id          UUID      PRIMARY KEY DEFAULT gen_random_uuid(),
    project_id          text                                   NOT NULL,
    cloud_provider      text                                   NOT NULL,
    region_id           text                                   NOT NULL,
    create_time         timestamp with time zone DEFAULT now() NOT NULL,
    name                text                                   NOT NULL,
    description         text,
    ipv4_cidr_block     cidr                                   NOT NULL,
    ipv6_cidr_block     cidr                                   NOT NULL,
    status              vpc.network_status DEFAULT 'CREATING'  NOT NULL,
    status_reason       text,
    external_id         text                                   NOT NULL,
    external_resources  jsonb              DEFAULT '{}'::jsonb NOT NULL,
    CONSTRAINT uniq_name_per_project UNIQUE (project_id, name)
);

CREATE TYPE vpc.subnet_status AS ENUM (
    'CREATING',
    'ACTIVE',
    'DELETING',
    'ERROR'
    );

CREATE TABLE vpc.subnets
(
    subnet_id           UUID      PRIMARY KEY DEFAULT gen_random_uuid(),
    network_id          UUID                                   NOT NULL,
    zone_id             text                                   NOT NULL,
    create_time         timestamp with time zone DEFAULT now() NOT NULL,
    ipv4_cidr_block     cidr                                   NOT NULL,
    ipv6_cidr_block     cidr                                   NOT NULL,
    status              vpc.subnet_status DEFAULT 'CREATING'   NOT NULL,
    status_reason       text,
    external_id         text                                   NOT NULL,
    CONSTRAINT uniq_zone_per_network UNIQUE (network_id, zone_id),
    CONSTRAINT fk_subnets_to_networks FOREIGN KEY (network_id)
        REFERENCES vpc.networks (network_id) ON DELETE CASCADE
);

CREATE TYPE vpc.operation_status AS ENUM (
    'PENDING',
    'RUNNING',
    'DONE'
    );

CREATE TABLE vpc.operations
(
    operation_id        UUID      PRIMARY KEY DEFAULT gen_random_uuid(),
    project_id          text                                   NOT NULL,
    description         text,
    created_by          text                                   NOT NULL,
    metadata            jsonb,
    create_time         timestamp with time zone DEFAULT now() NOT NULL,
    start_time          timestamp with time zone,
    finish_time         timestamp with time zone,
    status              vpc.operation_status DEFAULT 'PENDING' NOT NULL,
    state               jsonb              DEFAULT '{}'::jsonb NOT NULL,
    action              text                                   NOT NULL,
    cloud_provider      text                                   NOT NULL,
    region              text                                   NOT NULL
);

CREATE INDEX operation_status ON vpc.operations (status);

GRANT USAGE ON SCHEMA vpc TO vpc_worker;
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA vpc TO vpc_worker;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA vpc TO vpc_worker;
GRANT SELECT ON TABLE public.schema_version TO vpc_worker;

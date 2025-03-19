CREATE INDEX networks_project ON vpc.networks (project_id);

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
    project_id            text                                             NOT NULL,
    cloud_provider        text                                             NOT NULL,
    region_id             text                                             NOT NULL,
    create_time           timestamp with time zone      DEFAULT now()      NOT NULL,
    description           text,
    status                vpc.network_connection_status DEFAULT 'CREATING' NOT NULL,
    status_reason         text,
    connection_params     jsonb                         DEFAULT '{}'::jsonb NOT NULL
);

CREATE INDEX network_connections_project ON vpc.network_connections (project_id);
CREATE INDEX network_connections_network ON vpc.network_connections (network_id);

GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA vpc TO vpc_worker;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA vpc TO vpc_worker;

GRANT SELECT, INSERT, UPDATE ON vpc.network_connections TO vpc_api;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA vpc TO vpc_api;

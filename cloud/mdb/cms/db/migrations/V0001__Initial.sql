CREATE SCHEMA cms;

CREATE TYPE cms.action_name as enum (
    'prepare',
    'deactivate',
    'power-off',
    'reboot',
    'profile',
    'redeploy',
    'repair-link',
    'change-disk'
    );

CREATE TYPE cms.request_status as enum (
    'ok',
    'in-process',
    'rejected'
    );

CREATE TYPE cms.request_creation_type as enum (
    'manual',
    'automated'
    );

CREATE TABLE cms.requests
(
    id             bigint                                 NOT NULL GENERATED ALWAYS AS IDENTITY,
    name           cms.action_name                        NOT NULL,
    request_ext_id text                                   NOT NULL,
    status         cms.request_status                     NOT NULL,
    comment        text,
    author         text                                   NOT NULL,
    request_type   cms.request_creation_type              NOT NULL,
    extra          jsonb,
    fqdns          text[]                                 NOT NULL,
    created_at     timestamp with time zone default now() not null,
    resolved_at    timestamp with time zone,
    resolved_by    text,
    is_deleted     boolean                  DEFAULT false NOT NULL,

    CONSTRAINT pk_actions PRIMARY KEY (id),
    CONSTRAINT at_least_one_fqnd CHECK ( cardinality(fqdns) > 0 ),
    CONSTRAINT resolution_is_consistent CHECK (
            (resolved_at IS NULL AND resolved_by IS NULL)
            OR
            (resolved_at IS NOT NULL AND resolved_by IS NOT NULL)
        )
);

CREATE UNIQUE INDEX requests_request_ext_id ON cms.requests (request_ext_id);

CREATE INDEX requests_resolved_at ON cms.requests (resolved_at) WHERE (resolved_at IS NULL);

CREATE INDEX requests_is_not_deleted ON cms.requests (request_ext_id) WHERE NOT is_deleted;

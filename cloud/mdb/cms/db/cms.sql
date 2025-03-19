CREATE SCHEMA cms;

CREATE TYPE cms.action_name AS ENUM (
    'prepare',
    'deactivate',
    'power-off',
    'reboot',
    'profile',
    'redeploy',
    'repair-link',
    'temporary-unreachable',
    'change-disk'
    );

CREATE TYPE cms.request_creation_type AS ENUM (
    'manual',
    'automated'
    );

CREATE TYPE cms.request_status AS ENUM (
    'ok',
    'in-process',
    'rejected'
    );

CREATE TABLE cms.requests
(
    id                  bigint                                       NOT NULL,
    name                cms.action_name                              NOT NULL,
    request_ext_id      text                                         NOT NULL,
    status              cms.request_status                           NOT NULL,
    comment             text,
    author              text                                         NOT NULL,
    request_type        cms.request_creation_type                    NOT NULL,
    extra               jsonb,
    fqdns               text[]                                       NOT NULL,
    created_at          timestamp with time zone DEFAULT now()       NOT NULL,
    resolved_at         timestamp with time zone,
    resolved_by         text,
    is_deleted          boolean                  DEFAULT false       NOT NULL,
    resolve_explanation text                     DEFAULT ''          NOT NULL,
    analysed_by         text,
    came_back_at        timestamp with time zone,
    done_at             timestamp with time zone,
    failure_type        text                     DEFAULT ''          NOT NULL,
    scenario_info       jsonb                    DEFAULT '{}'::jsonb NOT NULL,
    CONSTRAINT resolution_is_consistent CHECK (((resolved_at IS NULL) AND (resolved_by IS NULL)) OR
                                               ((resolved_at IS NOT NULL) AND (resolved_by IS NOT NULL))),
    CONSTRAINT rejected_with_explanation CHECK
        ((status = 'rejected' AND resolve_explanation != '') OR (status != 'rejected')),
    CONSTRAINT at_least_one_fqdn CHECK ( cardinality(fqdns) > 0 )
);

ALTER TABLE cms.requests
    ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
        SEQUENCE NAME cms.requests_id_seq
            START WITH 1
            INCREMENT BY 1
            NO MINVALUE
            NO MAXVALUE
            CACHE 1
        );

SELECT pg_catalog.setval('cms.requests_id_seq', 1, false);

ALTER TABLE ONLY cms.requests
    ADD CONSTRAINT pk_actions PRIMARY KEY (id);

CREATE INDEX requests_resolved_at ON cms.requests USING btree (resolved_at) WHERE (resolved_at IS NULL);

CREATE UNIQUE INDEX requests_request_ext_id ON cms.requests USING btree (request_ext_id);

CREATE INDEX requests_is_not_deleted ON cms.requests (request_ext_id) WHERE NOT is_deleted;

CREATE INDEX requests_fqdns_is_deleted ON cms.requests USING gin (fqdns) WITH (fastupdate=off) WHERE is_deleted;

CREATE TYPE cms.decision_status AS ENUM (
    'new',
    'processing',
    'wait',
    'ok',
    'rejected',
    'at-wall-e',
    'before-done',
    'done',
    'escalated',
    'cleanup'
    );

CREATE TYPE cms.ad_resolution AS ENUM (
    'unknown',
    'approved',
    'rejected'
    );

CREATE TABLE cms.decisions
(
    id            bigint GENERATED ALWAYS AS IDENTITY NOT NULL,
    request_id    bigint                              NOT NULL,
    status        cms.decision_status                 NOT NULL,
    explanation   text                                NOT NULL,
    decided_at    timestamp with time zone            NOT NULL DEFAULT now(),
    ad_resolution cms.ad_resolution                   NOT NULL,
    mutations_log text,
    ops_metadata_log  jsonb NOT NULL,
    after_walle_log text NOT NULL,
    cleanup_log text NOT NULL DEFAULT '',

    CONSTRAINT pk_decisions PRIMARY KEY (id),
    CONSTRAINT uk_decision_request_id UNIQUE (request_id),
    CONSTRAINT decide_with_explanation CHECK (
        explanation != '' OR status NOT IN ('ok', 'rejected', 'escalated')
        ),
    CONSTRAINT fk_decisions_to_requests FOREIGN KEY (request_id)
        REFERENCES cms.requests (id) ON DELETE CASCADE
);

CREATE INDEX decisions_status ON cms.decisions (status);

CREATE EXTENSION pgcrypto;

CREATE TYPE cms.instance_operation_type AS ENUM (
    'move',
    'whip_primary_away'
    );

CREATE TYPE cms.instance_operation_status AS ENUM (
    'new',
    'in-progress',
    'ok-pending',
    'ok',
    'reject-pending',
    'rejected'
    );

CREATE TABLE cms.instance_operations
(
    operation_id     UUID      PRIMARY KEY DEFAULT gen_random_uuid(),
    idempotency_key  text                                   NOT NULL,
    operation_type   cms.instance_operation_type            NOT NULL,
    status           cms.instance_operation_status          NOT NULL,
    comment          text,
    author           text                                   NOT NULL,
    instance_id      text                                   NOT NULL,
    created_at       timestamp with time zone DEFAULT now() NOT NULL,
    modified_at      timestamp with time zone DEFAULT now() NOT NULL,
    explanation      text DEFAULT ''                        NOT NULL,
    operation_log    text DEFAULT ''                        NOT NULL,
    operation_state  jsonb                                  NOT NULL,
    executed_step_names text[] DEFAULT '{}' NOT NULL
        CONSTRAINT instance_resolution_is_consistent CHECK (
                (
                    (status != 'rejected')
                    ) OR (
                    (explanation != '')
                    )
            )
);

CREATE INDEX instance_operations_is_not_done ON cms.instance_operations (operation_id) WHERE status not in ('ok', 'rejected');
CREATE UNIQUE INDEX instance_operations_external_id ON cms.instance_operations USING btree (idempotency_key, instance_id);
CREATE INDEX operations_status ON cms.instance_operations (status);
CREATE UNIQUE INDEX one_running_operation ON cms.instance_operations (instance_id, operation_type) WHERE (status != 'ok');

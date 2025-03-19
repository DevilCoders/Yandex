CREATE EXTENSION pgcrypto;

CREATE TYPE cms.instance_operation_type AS ENUM (
    'move',
    'whip_primary_away'
    );

CREATE TYPE cms.instance_operation_status AS ENUM (
    'new',
    'in-progress',
    'ok',
    'rejected'
    );

CREATE TABLE cms.instance_operations
(
    operation_id        UUID      PRIMARY KEY DEFAULT gen_random_uuid(),
    idempotency_key     text                                   NOT NULL,
    operation_type      cms.instance_operation_type            NOT NULL,
    status              cms.instance_operation_status          NOT NULL,
    comment             text,
    author              text                                   NOT NULL,
    instance_id         text                                   NOT NULL,
    created_at          timestamp with time zone DEFAULT now() NOT NULL,
    modified_at         timestamp with time zone DEFAULT now() NOT NULL,
    explanation         text                     DEFAULT ''    NOT NULL,
    operation_log       text                     DEFAULT ''    NOT NULL,
    operation_state     jsonb                                  NOT NULL,
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

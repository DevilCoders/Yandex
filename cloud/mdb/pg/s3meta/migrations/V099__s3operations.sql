CREATE TYPE s3.operation_status AS ENUM (
    'new',
    'running',
    'finished',
    'failed',
    'canceled'
);

CREATE TABLE s3.operations (
    id TEXT NOT NULL,
    bucket_name TEXT NOT NULL,
    service_id BIGINT NOT NULL,
    idempotency_key TEXT,
    background_task_id uuid,
    created_at TIMESTAMP NOT NULL DEFAULT now(),
    updated_at TIMESTAMP NOT NULL DEFAULT now(),
    created_by TEXT NOT NULL,
    description TEXT NOT NULL,
    status s3.operation_status NOT NULL,
    kind TEXT NOT NULL,
    version TEXT NOT NULL,
    payload JSONB,
    CONSTRAINT pk_operations PRIMARY KEY(id),
    CONSTRAINT fk_operations_service_id_accounts FOREIGN KEY (service_id)
        REFERENCES s3.accounts ON DELETE CASCADE
);

CREATE UNIQUE INDEX operations_idempotency_key_idx ON s3.operations USING btree(idempotency_key) WHERE idempotency_key IS NOT NULL;
CREATE INDEX operations_service_id_idx ON s3.operations USING btree(service_id);
CREATE INDEX operations_status_idx ON s3.operations USING btree(status) WHERE status = 'running';

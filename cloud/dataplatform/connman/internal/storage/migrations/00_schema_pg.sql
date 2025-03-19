CREATE TABLE connection
(
    id          text      NOT NULL,
    folder_id   text      NOT NULL,
    created_at  timestamp NOT NULL DEFAULT NOW(),
    updated_at  timestamp,
    deleted_at  timestamp,
    name        text      NOT NULL,
    description text,
    labels      jsonb,
    created_by  text      NOT NULL,
    params      bytea     NOT NULL,
    CONSTRAINT connection_pkey PRIMARY KEY (id),
    CONSTRAINT connection_folder_id_name_key UNIQUE (folder_id, name)
);
CREATE INDEX connection_created_by_idx ON connection (created_by);

CREATE TABLE connection_operation
(
    id            text      NOT NULL,
    connection_id text      NOT NULL,
    created_at    timestamp NOT NULL DEFAULT NOW(),
    updated_at    timestamp,
    created_by    text      NOT NULL,
    done          bool      NOT NULL DEFAULT FALSE,
    metadata      bytea     NOT NULL,
    CONSTRAINT connection_operation_pkey PRIMARY KEY (id),
    CONSTRAINT connection_operation_connection_id_fkey FOREIGN KEY (connection_id) REFERENCES connection (id)
);
CREATE INDEX connection_operation_created_by_idx ON connection_operation (created_by);

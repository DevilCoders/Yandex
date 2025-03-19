CREATE SCHEMA mlock;

CREATE TABLE mlock.objects (
    object_id bigint NOT NULL GENERATED ALWAYS AS IDENTITY,
    object_name text NOT NULL,

    CONSTRAINT pk_objects PRIMARY KEY (object_id),
    CONSTRAINT uk_objects_object_name UNIQUE (object_name),

    CONSTRAINT check_object_name_length CHECK (
        char_length(object_name) BETWEEN 1 AND 256
    )
);

CREATE TABLE mlock.locks (
    lock_id bigint NOT NULL GENERATED ALWAYS AS IDENTITY,
    try_count bigint NOT NULL DEFAULT 1,
    create_ts timestamptz NOT NULL DEFAULT now(),
    lock_ext_id text NOT NULL,
    holder text NOT NULL,
    reason text NOT NULL,

    CONSTRAINT pk_locks PRIMARY KEY (lock_id),
    CONSTRAINT uk_locks_lock_ext_id UNIQUE (lock_ext_id),

    CONSTRAINT check_lock_ext_id_length CHECK (
        char_length(lock_ext_id) BETWEEN 1 AND 256
    ),
    CONSTRAINT check_holder_length CHECK (
        char_length(holder) BETWEEN 1 AND 256
    ),
    CONSTRAINT check_reason_length CHECK (
        char_length(reason) BETWEEN 1 AND 1024
    )
);

CREATE TABLE mlock.object_locks (
    lock_id bigint NOT NULL,
    object_id bigint NOT NULL,
    contend_order bigint NOT NULL,

    CONSTRAINT pk_object_locks PRIMARY KEY (lock_id, object_id, contend_order),
    CONSTRAINT uk_object_locks_object_id_contend_order UNIQUE (object_id, contend_order),

    CONSTRAINT check_contend_order CHECK (
        contend_order > 0
    )
);

CREATE TYPE code.lock_status AS (
    lock_ext_id text,
    holder      text,
    reason      text,
    create_ts   timestamptz,
    objects     text[],
    acquired    boolean,
    conflicts   jsonb
);

CREATE TYPE code.lock AS (
    lock_ext_id text,
    holder      text,
    reason      text,
    create_ts   timestamptz,
    objects     text[]
);

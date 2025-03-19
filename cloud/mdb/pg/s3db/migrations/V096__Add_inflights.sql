CREATE TABLE s3.inflights
(
    bid              uuid    NOT NULL,

    name             text    NOT NULL,
    object_created   timestamp with time zone NOT NULL,
    inflight_created timestamp with time zone NOT NULL,

    part_id          integer NOT NULL,

    mds_couple_id    integer,
    mds_key_version  integer,
    mds_key_uuid     uuid,

    metadata         JSONB
);

CREATE UNIQUE INDEX pk_inflights ON s3.inflights
    (bid, name, object_created, inflight_created, part_id);

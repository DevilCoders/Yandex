CREATE TABLE s3.parts (
    part_id int NOT NULL PRIMARY KEY,
    shard_id int NOT NULL,
    new_chunks_allowed boolean NOT NULL DEFAULT true,
    UNIQUE (shard_id)
);

CREATE OR REPLACE FUNCTION s3.update_parts()
RETURNS SETOF s3.parts
LANGUAGE sql VOLATILE AS $function$
    INSERT INTO s3.parts (part_id, shard_id)
        SELECT p.part_id, row_number() OVER (ORDER BY p.part_id) - 1
            FROM parts p
                JOIN clusters c ON p.cluster_id = c.cluster_id AND c.name = 'db'
    ON CONFLICT (part_id) DO UPDATE SET shard_id = EXCLUDED.shard_id
    RETURNING *;
$function$;

SELECT * FROM s3.update_parts();

ALTER TABLE s3.chunks ADD COLUMN shard_id int REFERENCES s3.parts (shard_id),
    DROP CONSTRAINT idx_exclude_bid_key_range,
    ADD CONSTRAINT idx_exclude_bid_key_range
        EXCLUDE USING gist( (bid::text) WITH =, s3.to_keyrange(start_key, end_key) WITH &&)
        DEFERRABLE INITIALLY DEFERRED;
UPDATE s3.chunks SET shard_id = hashtext(format('%s-%s', bid::text, cid)) & (SELECT count(shard_id) - 1 FROM s3.parts);
ALTER TABLE s3.chunks ALTER COLUMN shard_id SET NOT NULL;

CREATE MATERIALIZED VIEW s3.shard_stat (
    shard_id,
    new_chunks_allowed,
    buckets_count,
    chunks_count
) AS SELECT
        s.shard_id,
        bool_and(s.new_chunks_allowed),
        count(distinct(bid)),
        count(distinct(bid, cid))
            FILTER (WHERE c.bid IS NOT NULL AND c.cid IS NOT NULL)
    FROM s3.parts s
        LEFT JOIN s3.chunks c USING (shard_id)
    GROUP BY s.shard_id;

CREATE UNIQUE INDEX ON s3.shard_stat (shard_id);

CREATE TABLE s3.chunks_create_queue (
    bid uuid not null,
    cid bigint not null,
    created timestamptz not null,
    start_key text,
    end_key text,
    shard_id int,
    PRIMARY KEY (bid, cid),
    FOREIGN KEY (bid, cid) REFERENCES s3.chunks (bid, cid) ON DELETE CASCADE
);

CREATE INDEX ON s3.chunks_create_queue USING btree (created);

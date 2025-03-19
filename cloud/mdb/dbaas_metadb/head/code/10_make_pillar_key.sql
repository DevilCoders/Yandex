CREATE OR REPLACE FUNCTION code.make_pillar_key(
    i_cid      text DEFAULT NULL,
    i_subcid   text DEFAULT NULL,
    i_shard_id text DEFAULT NULL,
    i_fqdn     text DEFAULT NULL
) RETURNS code.pillar_key
AS $$
SELECT (i_cid, i_subcid, i_shard_id, i_fqdn)::code.pillar_key;
$$ LANGUAGE SQL IMMUTABLE;

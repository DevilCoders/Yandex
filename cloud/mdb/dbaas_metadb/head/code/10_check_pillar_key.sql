CREATE OR REPLACE FUNCTION code.check_pillar_key(
	i_key code.pillar_key
) RETURNS void AS $$
BEGIN
-- Since PostgreSQL 11. We can CREATE DOMAIN on composite type, instead that function.
-- But we don't upgrade MetaDB yet.
--
-- CREATE DOMAIN code.pillar_key AS code.pillar_key_record
--    CONSTRAINT all_keys_is_not_null NOT NULL
--    CONSTRAINT exists_only_one_not_null_key CHECK (
--         (
--             ((VALUE).cid IS NOT NULL)::int +
--             ((VALUE).subcid IS NOT NULL)::int +
--             ((VALUE).shard_id IS NOT NULL)::int +
--             ((VALUE).fqdn IS NOT NULL)::int
--         ) = 1
--    );
    IF $1 IS NULL THEN
        RAISE EXCEPTION 'All pillar-components are null: %', to_json($1);
    END IF;

	IF ((($1).cid      IS NOT NULL)::int +
        (($1).subcid   IS NOT NULL)::int +
        (($1).shard_id IS NOT NULL)::int +
        (($1).fqdn     IS NOT NULL)::int) != 1
	THEN
        RAISE EXCEPTION 'Only one pillar-key component should be defined: %', to_json($1);
    END IF;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE TYPE s3.keyrange AS RANGE (
    SUBTYPE = text,
    COLLATION = "C"
);

CREATE FUNCTION s3.to_keyrange(
    start_key text,
    end_key text
) RETURNS s3.keyrange
LANGUAGE plpgsql IMMUTABLE AS
$function$
BEGIN
    RETURN format('[%s,%s)', start_key, end_key)::s3.keyrange;
END;
$function$;

CREATE EXTENSION btree_gist;

ALTER TABLE ONLY s3.chunks
    ALTER COLUMN start_key TYPE text COLLATE "C",
    ALTER COLUMN end_key TYPE text COLLATE "C",
    ADD CONSTRAINT check_key_range_empty CHECK (NOT isempty(s3.to_keyrange(start_key, end_key))),
    ADD CONSTRAINT idx_exclude_bid_key_range
        EXCLUDE USING gist( (bid::text) WITH =, s3.to_keyrange(start_key, end_key) WITH &&);

CREATE AGGREGATE s3.keyrange_union_agg(
    BASETYPE = s3.keyrange,
    SFUNC = range_union,
    STYPE = s3.keyrange
);

CREATE FUNCTION s3.chunk_check_keyrange() RETURNS TRIGGER
LANGUAGE plpgsql AS
$function$
DECLARE
    v_keyrange s3.keyrange;
    v_row s3.chunks;
BEGIN
    IF TG_OP = 'DELETE' THEN
        v_row = OLD;
    ELSE
        v_row = NEW;
    END IF;

    RAISE NOTICE 'Checking key range for chunk (%, %), start_key %, end_key %',
                    v_row.bid, v_row.cid, v_row.start_key, v_row.end_key;
    SELECT s3.keyrange_union_agg(
                s3.to_keyrange(start_key, end_key)
                    ORDER BY lower(s3.to_keyrange(start_key, end_key))
            ) INTO STRICT v_keyrange
        FROM s3.chunks WHERE bid = v_row.bid;
    IF NOT (lower_inf(v_keyrange) AND upper_inf(v_keyrange)) THEN
        RAISE EXCEPTION 'Key range of bucket % is not full, new range %',
                        v_row.bid, v_keyrange;
    END IF;
    -- RETURN value of AFTER trigger is always ignored
    RETURN NULL;
END;
$function$;

CREATE CONSTRAINT TRIGGER tg_chunk_check_interval
    AFTER INSERT OR DELETE OR UPDATE OF start_key, end_key
    ON s3.chunks
    DEFERRABLE INITIALLY DEFERRED
    FOR EACH ROW EXECUTE PROCEDURE s3.chunk_check_keyrange();

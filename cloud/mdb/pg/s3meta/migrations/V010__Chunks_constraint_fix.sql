CREATE OR REPLACE FUNCTION s3.chunk_check_keyrange() RETURNS TRIGGER
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
                    ORDER BY start_key NULLS FIRST
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

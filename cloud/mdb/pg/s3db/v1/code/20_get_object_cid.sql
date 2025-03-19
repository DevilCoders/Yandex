CREATE OR REPLACE FUNCTION v1_code.get_object_cid(
    i_bid uuid,
    i_name text
) RETURNS bigint
LANGUAGE plpgsql AS $function$
DECLARE
    v_cid bigint;
BEGIN
    /*
     * NOTE: We need to block object's chunk with KEY SHARE clause.
     * This is needed to prevent race condition with chunk splitting.
     */
    SELECT cid INTO v_cid FROM s3.chunks
        WHERE (bid = i_bid)
            AND (coalesce(start_key, '') <= i_name)
            AND (end_key IS NULL OR i_name < end_key)
    LIMIT 1
    FOR KEY SHARE;

    IF NOT FOUND THEN
        /*
         * NOTE: We need to use READ COMMITTED feature and try
         * to find chunk again because maybe chunk splitter changed
         * key ranges while we were waiting for a lock (FOR KEY SHARE)
         * and now i_name is out of [start_key, end_key) but new
         * chunk is absent in our shapshot. We need to repeat the
         * statement with new snapshot.
         */

        SELECT cid INTO v_cid FROM s3.chunks
            WHERE (bid = i_bid)
                AND (coalesce(start_key, '') <= i_name)
                AND (end_key IS NULL OR i_name < end_key)
        LIMIT 1
        FOR KEY SHARE;

        IF NOT FOUND THEN
            RAISE EXCEPTION 'No such chunk'
                USING ERRCODE = 'S3X01';
        END IF;
    END IF;

    RETURN v_cid;
END
$function$;

CREATE OR REPLACE FUNCTION v1_code.get_object_cid_non_blocking(
    i_bid uuid,
    i_name text,
    i_raise_not_found BOOLEAN DEFAULT TRUE
) RETURNS bigint
LANGUAGE plpgsql AS $function$
DECLARE
    v_cid bigint;
BEGIN
    /*
     * NOTE: We need to block object's chunk with KEY SHARE clause.
     * This is needed to prevent race condition with chunk splitting.
     */
    SELECT cid INTO v_cid FROM s3.chunks
        WHERE (bid = i_bid)
            AND (coalesce(start_key, '') <= i_name)
            AND (end_key IS NULL OR i_name < end_key)
    LIMIT 1;

    IF NOT FOUND THEN
        IF i_raise_not_found THEN
            RAISE EXCEPTION 'No such chunk'
                USING ERRCODE = 'S3X01';
        ELSE
            RETURN -1;
        END IF;
    END IF;

    RETURN v_cid;
END
$function$;

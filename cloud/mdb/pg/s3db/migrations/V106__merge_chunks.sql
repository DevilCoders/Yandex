CREATE OR REPLACE FUNCTION s3.chunk_check_empty() RETURNS TRIGGER
    LANGUAGE plpgsql AS
$function$
DECLARE
    v_keyrange s3.keyrange;
BEGIN
    IF TG_OP = 'UPDATE' THEN
        IF NEW.end_key < OLD.end_key THEN
            PERFORM true
            FROM s3.chunks
                 -- Use exclude constraint gist index (bid::text, s3.to_keyrange)
            WHERE bid::text = NEW.bid::text
              AND s3.to_keyrange(start_key, end_key) = s3.to_keyrange(NEW.end_key, OLD.end_key);
            IF NOT FOUND THEN
                RAISE EXCEPTION 'Incorrect update keys of chunk (%, %, %, %)'
                    ': chunk (%, %) not found', OLD.bid, OLD.cid,
                    OLD.start_key, OLD.end_key, NEW.end_key, OLD.end_key;
            END IF;
        END IF;

        IF NEW.start_key > OLD.start_key THEN
            PERFORM true
            FROM s3.chunks
                 -- Use exclude constraint gist index (bid::text, s3.to_keyrange)
            WHERE bid::text = NEW.bid::text
              AND s3.to_keyrange(start_key, end_key) = s3.to_keyrange(OLD.start_key, NEW.start_key);
            IF NOT FOUND THEN
                RAISE EXCEPTION 'Incorrect update keys of chunk (%, %, %, %)'
                    ': chunk (%, %) not found', OLD.bid, OLD.cid,
                    OLD.start_key, OLD.end_key, OLD.start_key, NEW.start_key;
            END IF;
        END IF;
    ELSIF TG_OP = 'DELETE' THEN

        RAISE NOTICE 'Checking emptiness for chunk (%, %, %, %)',
            OLD.bid, OLD.cid, OLD.start_key, OLD.end_key;
        /*
         * We do not need to do anything about locking to prevent
         * race conditions with insert into s3.objects and delete
         * from s3.chunks here. Because any insert into s3.objects
         * obtain share lock on s3.chunks row and here s3.chunks
         * row already locked.
         */
        EXECUTE format($$
            SELECT true FROM s3.objects
                WHERE bid = %1$L /* bid */
                    AND (%2$L IS NULL OR name >= %2$L) /* start_key */
                    AND (%3$L IS NULL OR name < %3$L) /* end_key */
            LIMIT 1
        $$, OLD.bid, OLD.start_key, OLD.end_key)
            INTO FOUND;
        IF FOUND THEN
            RAISE EXCEPTION 'Chunk (%, %, %, %) is not empty',
                OLD.bid, OLD.cid, OLD.start_key, OLD.end_key
                USING ERRCODE = 23503;
        END IF;
    ELSE
        RAISE EXCEPTION 'This funciton must be called only on DELETE or UPDATE';
    END IF;

    RETURN OLD;
END;
$function$;

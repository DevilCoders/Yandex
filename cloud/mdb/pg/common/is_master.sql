DROP FUNCTION IF EXISTS public.is_master(bigint);

CREATE OR REPLACE FUNCTION public.is_master(i_key bigint)
RETURNS boolean
LANGUAGE plpgsql AS
$function$
DECLARE
    read_only boolean;
    closed boolean;
BEGIN
    closed := current_setting('pgcheck.closed', true);
    IF closed IS true
    THEN
        RAISE EXCEPTION 'database closed from load (pgcheck.closed = %)', closed;
    END IF;
    read_only := current_setting('transaction_read_only');
    RETURN NOT read_only;
END
$function$;

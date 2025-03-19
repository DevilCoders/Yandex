CREATE OR REPLACE FUNCTION show_activity_stats(state_filter text)
RETURNS bigint AS $$
DECLARE res bigint;
BEGIN
    SELECT count(*) INTO res FROM pg_stat_activity WHERE state = state_filter;
    RETURN res;
END;
$$ LANGUAGE PLPGSQL
   SECURITY DEFINER
   SET search_path = public, pg_temp;

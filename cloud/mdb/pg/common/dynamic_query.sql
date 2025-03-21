CREATE OR REPLACE FUNCTION public.dynamic_query(sql text)
RETURNS SETOF RECORD AS $$
DECLARE
    rec RECORD;
BEGIN
    FOR rec IN EXECUTE sql
    LOOP
        RETURN NEXT rec;
    END LOOP;
    RETURN;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_unapplied_settings()
RETURNS TABLE (
    name text,
    error text,
    sourceline integer
) AS $$ 
BEGIN

RETURN QUERY SELECT s.name, s.error, s.sourceline 
    FROM pg_file_settings s
    WHERE NOT applied AND s.error IS NOT NULL;
END;
$$ LANGUAGE PLPGSQL
    SECURITY DEFINER;

CREATE OR REPLACE FUNCTION mdb_terminate_backend(pid integer)
RETURNS boolean AS $$
DECLARE ret boolean;
BEGIN
    SELECT pg_terminate_backend(pid) INTO ret;
    RETURN ret;
END;
$$ LANGUAGE PLPGSQL
   SECURITY DEFINER;

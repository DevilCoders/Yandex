CREATE OR REPLACE FUNCTION code._acquire_master(
    i_fqdn text
) RETURNS deploy.masters AS $$
DECLARE
    v_masters deploy.masters;
BEGIN
    SELECT * INTO v_masters
      FROM deploy.masters
    WHERE fqdn = i_fqdn
      FOR UPDATE;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find master %', i_fqdn
            USING ERRCODE = code._error_not_found(), TABLE = 'deploy.masters';
    END IF;

    RETURN v_masters;
END;
$$ LANGUAGE plpgsql;

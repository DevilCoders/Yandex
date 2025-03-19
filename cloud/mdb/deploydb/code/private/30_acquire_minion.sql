CREATE OR REPLACE FUNCTION code._acquire_minion(
    i_fqdn text
) RETURNS deploy.minions AS $$
DECLARE
    v_minion deploy.minions;
BEGIN
    SELECT *
      INTO v_minion
      FROM deploy.minions
     WHERE fqdn = i_fqdn
       FOR UPDATE;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find minion %', i_fqdn
                  USING ERRCODE = code._error_not_found(), TABLE = 'deploy.minions';
    END IF;

    RETURN v_minion;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION code._get_group(
    i_name text
) RETURNS deploy.groups AS $$
DECLARE
    v_group deploy.groups;
BEGIN
    SELECT *
      INTO v_group
      FROM deploy.groups
     WHERE name = i_name;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find group with name %', i_name
              USING ERRCODE = code._error_not_found(), TABLE = 'deploy group';
    END IF;

    RETURN v_group;
END;
$$ LANGUAGE plpgsql STABLE;

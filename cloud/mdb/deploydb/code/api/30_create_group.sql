CREATE OR REPLACE FUNCTION code.create_group(
    i_name         text
) RETURNS code.deploy_group AS $$
DECLARE
    v_group deploy.groups;
BEGIN
    INSERT INTO deploy.groups
        (name)
    VALUES
        (i_name)
    RETURNING * INTO v_group;

    RETURN code._as_deploy_group(v_group);
END;
$$ LANGUAGE plpgsql;
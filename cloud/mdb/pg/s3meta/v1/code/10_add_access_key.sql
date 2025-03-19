CREATE OR REPLACE FUNCTION v1_code.add_access_key(
    i_service_id bigint,
    i_user_id bigint,
    i_role s3.role_type,
    i_key_id text,
    i_secret_token text,
    i_key_version integer
) RETURNS v1_code.access_key
LANGUAGE plpgsql AS $function$
DECLARE
    v_access_keys_count bigint;
    v_access_key v1_code.access_key;
BEGIN
    /*
     * NOTE: Lock corresponding granted role to ensure its existence and ensure
     * that no access key for the same granted role will be added during this
     * transaction. The latter is needed to perform check on maximum number of
     * acccess keys per service role.
     */
    PERFORM 1
        FROM s3.granted_roles
        WHERE service_id = i_service_id
          AND grantee_uid = i_user_id
          AND role = i_role
        FOR UPDATE;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'No such service role'
            USING ERRCODE = 'S3A02';
    END IF;

    SELECT COUNT(*) INTO v_access_keys_count
        FROM s3.access_keys
        WHERE service_id = i_service_id
          AND user_id = i_user_id
          AND role = i_role;
    /*
     * NOTE: Limit maximum number of access keys per service role.
     */
    IF v_access_keys_count >= 5 THEN
        RAISE EXCEPTION 'Service role has reached maximum number of access keys'
            USING ERRCODE = 'S3A05';
    END IF;

    INSERT INTO s3.access_keys(service_id, user_id, role, key_id, secret_token, key_version)
        VALUES (i_service_id, i_user_id, i_role, i_key_id, i_secret_token, i_key_version)
        RETURNING service_id, user_id, role, key_id, secret_token, key_version, issue_date
            INTO v_access_key;

    RETURN v_access_key;
END
$function$;

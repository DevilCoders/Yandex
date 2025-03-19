CREATE OR REPLACE FUNCTION v1_code.delete_access_key(
    i_service_id bigint,
    i_user_id bigint,
    i_role s3.role_type,
    i_key_id text
) RETURNS v1_code.access_key
LANGUAGE plpgsql AS $function$
DECLARE
    v_access_key v1_code.access_key;
BEGIN
    DELETE FROM s3.access_keys
        WHERE service_id = i_service_id
          AND user_id = i_user_id
          AND role = i_role
          AND key_id = i_key_id
        RETURNING service_id, user_id, role, key_id, secret_token, key_version, issue_date
            INTO v_access_key;

    /*
     * NOTE: Requested access key could not be found due to invalid service_id,
     * user_id or role arguments, but still consider this as InvalidAccessKeyID
     * error.
     */
    IF NOT FOUND THEN
        RAISE EXCEPTION 'Invalid access key ID'
            USING ERRCODE = 'S3A04';
    END IF;

    RETURN v_access_key;
END
$function$;

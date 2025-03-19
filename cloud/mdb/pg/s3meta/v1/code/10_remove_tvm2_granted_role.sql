/*
 * Removes granted role from the specified grantee (TVM2 app) within the service
 * account.
 *
 * Args:
 * - i_service_id:
 *     ID of the service account (grantor).
 * - i_role:
 *     Specifies the role to be removed.
 * - i_grantee_uid:
 *     ID of the TVM2 app (grantee).
 *
 * Raises:
 * - S3A01 (AccessDenied):
 *     If 'service_id' was not provided (i.e. it's value is NULL).
 * - S3A02 (NotSignedUp):
 *     If the specified service has no account (i.e. it wasn't registered).
 */
CREATE OR REPLACE FUNCTION v1_code.remove_tvm2_granted_role(
    i_service_id bigint,
    i_role s3.role_type,
    i_grantee_uid bigint
) RETURNS v1_code.granted_role
LANGUAGE plpgsql AS
$$
DECLARE
    v_account v1_code.account;
    v_granted_role v1_code.granted_role;
BEGIN
    IF i_service_id IS NULL THEN
        RAISE EXCEPTION 'Anonymous access is forbidden'
            USING ERRCODE = 'S3A01';
    END IF;

    SELECT service_id, status, registration_date
        INTO v_account
        FROM s3.accounts
        WHERE service_id = i_service_id;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'No such service account'
            USING ERRCODE = 'S3A02';
    END IF;

    DELETE FROM s3.tvm2_granted_roles
        WHERE service_id = i_service_id
          AND role = i_role
          AND grantee_uid = i_grantee_uid
        RETURNING service_id, role, grantee_uid, issue_date
            INTO v_granted_role;

    RETURN v_granted_role;
END;
$$;

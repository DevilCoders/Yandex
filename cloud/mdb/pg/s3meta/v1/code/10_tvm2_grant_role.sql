/*
 * Grants the specified role to a grantee (TVM2 app) within the service account.
 *
 * Args:
 * - i_service_id:
 *     ID of the service account (grantor).
 * - i_role:
 *     Specifies the role to be granted.
 * - i_grantee_uid:
 *     ID of the TVM2 app (grantee).
 *
 * Raises:
 * - S3A01 (AccessDenied):
 *     If 'service_id' was not provided (i.e. it's value is NULL).
 * - S3A02 (NotSignedUp):
 *     If the specified service has no account (i.e. it wasn't registered).
 * - S3A03 (AccountProblem):
 *     If the service account is in invalid state for this operation. Account
 *     must be 'active' to be able to grant roles.
 */
CREATE OR REPLACE FUNCTION v1_code.tvm2_grant_role(
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

    IF v_account.status != 'active' THEN
        RAISE EXCEPTION 'Account problem'
            USING ERRCODE = 'S3A03';
    END IF;

    /*
     * NOTE: Granting role should be idempotent. Here "ON CONFLICT DO UPDATE"
     * with dummy "SET" action is used to make the row with granted role to be
     * returned.
     */
    INSERT INTO s3.tvm2_granted_roles (service_id, role, grantee_uid)
        VALUES (i_service_id, i_role, i_grantee_uid)
        ON CONFLICT (service_id, role, grantee_uid) DO UPDATE SET
            grantee_uid = i_grantee_uid
        RETURNING service_id, role, grantee_uid, issue_date
            INTO v_granted_role;

    RETURN v_granted_role;
END;
$$;

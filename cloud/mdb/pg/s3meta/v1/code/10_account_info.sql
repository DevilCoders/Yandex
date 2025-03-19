/*
 * Returns the service account's info.
 *
 * Args:
 * - i_service_id:
 *     ID of the service account to get info about.
 *
 * Raises:
 * - S3A01 (AccessDenied):
 *     If 'service_id' was not provided (i.e. it's value is NULL).
 * - S3A02 (NotSignedUp):
 *     If the specified service has no account (i.e. it wasn't registered).
 */
CREATE OR REPLACE FUNCTION v1_code.account_info(
    i_service_id bigint
) RETURNS v1_code.account
LANGUAGE plpgsql STABLE AS
$$
DECLARE
    v_account v1_code.account;
BEGIN
    IF i_service_id IS NULL THEN
        RAISE EXCEPTION 'Anonymous access is forbidden'
            USING ERRCODE = 'S3A01';
    END IF;

    SELECT service_id, status, registration_date, max_size, max_buckets, folder_id, cloud_id
        INTO v_account
        FROM s3.accounts
        WHERE service_id = i_service_id;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'No such service account'
            USING ERRCODE = 'S3A02';
    END IF;

    RETURN v_account;
END;
$$;

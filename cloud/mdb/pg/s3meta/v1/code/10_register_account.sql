/*
 * Registers service account.
 *
 * Args:
 * - i_service_id:
 *     ID of the service to register.
 * - i_max_size:
 *     Maximum total size (in bytes) of all the objects in all account buckets.
 *     By default NULL is used, i.e. account has no restriction.
 * - i_max_buckets:
 *     Maximum total count of buckets in account.
 *     By default NULL is used, i.e. account has no restriction.
 * - i_folder_id:
 *     ID of YC folder. By default seting to text representation of i_service_id.
 * - i_cloud_id:
 *     ID of YC cloud.
 *
 * Raises:
 * - S3A01 (AccessDenied):
 *     If 'service_id' was not provided (i.e. it's value is NULL).
 *
 * NOTE: If the service account was already registered, but was suspended, this
 * will "re-activate" the account.
 */
CREATE OR REPLACE FUNCTION v1_code.register_account(
    i_service_id bigint,
    i_max_size bigint DEFAULT NULL,
    i_max_buckets bigint DEFAULT NULL
) RETURNS v1_code.account
LANGUAGE plpgsql AS
$$
DECLARE
    v_account v1_code.account;
BEGIN
    IF i_service_id IS NULL THEN
        RAISE EXCEPTION 'Anonymous access is forbidden'
            USING ERRCODE = 'S3A01';
    END IF;

    /*
     * NOTE: Registering service account should be idempotent and "re-activate"
     * account if it was suspended.
     */
    INSERT INTO s3.accounts (service_id, max_size, max_buckets, folder_id)
        VALUES (
            i_service_id,
            i_max_size,
            i_max_buckets,
            cast(i_service_id as text)
        )
        ON CONFLICT (service_id) DO UPDATE SET
            status = 'active',
            folder_id = cast(i_service_id as text)
        RETURNING service_id, status, registration_date, max_size, max_buckets, folder_id, cloud_id
            INTO v_account;

    RETURN v_account;
END;
$$;

CREATE OR REPLACE FUNCTION v1_code.register_account(
    i_folder_id text,
    i_cloud_id text
) RETURNS v1_code.account
LANGUAGE plpgsql AS
$$
DECLARE
    v_service_id bigint := nextval('s3.auto_account_id');
    v_account v1_code.account;
BEGIN
    IF i_folder_id IS NULL THEN
        RAISE EXCEPTION 'Anonymous access is forbidden'
            USING ERRCODE = 'S3A01';
    END IF;

    INSERT INTO s3.accounts (service_id, folder_id, cloud_id)
        VALUES (
            v_service_id,
            i_folder_id,
            i_cloud_id
        )
        ON CONFLICT (folder_id) DO UPDATE SET
            cloud_id = i_cloud_id
        RETURNING service_id, status, registration_date, max_size, max_buckets, folder_id, cloud_id
            INTO v_account;

    RETURN v_account;
END;
$$;

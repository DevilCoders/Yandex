/*
 * Drops all service accounts with all their granted roles.
 *
 * NOTE: This function is intended to be used in tests to clear accounts DB.
 */
CREATE OR REPLACE FUNCTION v1_impl.drop_accounts()
RETURNS void
LANGUAGE sql AS
$$
    DELETE FROM s3.granted_roles;
    DELETE FROM s3.tvm2_granted_roles;
    DELETE FROM s3.accounts;
$$;

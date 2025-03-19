/*
 * Returns the bucket's object (or the specific version of the object).
 * It is a copy (proxy) of base function v1_code.object_info
 * It should to be called only on master to evade replicas lag
 * Copy is made to be consistent with usage of plproxy
 *
 * Args:
 * - i_bucket_name, i_bid:
 *     Name and ID of the bucket to get the object from.
 * - i_name:
 *     Name of the object to get.
 * - i_created, i_null_version:
 *     Specifies the version of the object to get as follows:
 *     - if ``i_created`` is set to NULL and ``i_null_version`` is set to false,
 *       the current version of the object is returned,
 *     - if ``i_created`` is specified, the object's version with the same
 *       creation date is returned,
 *     - if ``i_null_version`` is set to true, the object's 'null' version is
 *       returned.
 *
 * Returns:
 *   An instance of ``code.object`` that represents the object's version, or
 *   empty set of ``code.object`` if there's no such object's version.
 */
CREATE OR REPLACE FUNCTION v1_code.actual_object_info(
    i_bucket_name text,
    i_bid uuid,
    i_name text,
    i_created timestamp with time zone DEFAULT NULL,
    i_null_version boolean DEFAULT false
) RETURNS SETOF v1_code.object
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY
        SELECT * FROM v1_code.object_info(
            i_bucket_name,
            i_bid,
            i_name,
            i_created,
            i_null_version);
END;
$$;

/*
 * Returns list of all service account's buckets.
 *
 * Args:
 * - i_service_id:
 *     ID of the service account. By default NULL is used as a service ID and
 *     all "publicly accessible" buckets are listed.
 * - i_list_all:
 *     Flag to get all buckets on the shard indifferent of service_id.
 */
CREATE OR REPLACE FUNCTION v1_code.list_buckets(
    i_service_id bigint DEFAULT NULL,
    i_list_all   boolean DEFAULT false
) RETURNS SETOF v1_code.bucket_header LANGUAGE plpgsql AS
$$
BEGIN
    IF i_list_all IS true THEN
        RETURN QUERY
            SELECT bid, name, created, versioning, banned, service_id, max_size,
                    anonymous_read, anonymous_list
                FROM s3.buckets;
    ELSE
        RETURN QUERY
            SELECT bid, name, created, versioning, banned, service_id, max_size,
                    anonymous_read, anonymous_list
                FROM s3.buckets
                WHERE service_id IS NOT DISTINCT FROM i_service_id
                AND state = 'alive';
    END IF;
END;
$$;


CREATE OR REPLACE FUNCTION v1_code.abort_multiple_uploads(
    i_bid uuid,
    i_uploads v1_code.lifecycle_element_key[]
) RETURNS SETOF v1_code.lifecycle_result
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY WITH keys AS (
        SELECT DISTINCT i_bid as bid, name, created, v1_code.get_object_cid(i_bid, name) as cid
        FROM unnest(i_uploads)
    ), parts AS (
        DELETE FROM s3.object_parts u
            USING keys as k
            WHERE u.bid = i_bid
                AND u.bid = k.bid
                AND u.name = k.name
                AND u.object_created = k.created
        RETURNING (u.bid, k.cid, u.name, u.object_created, u.part_id, u.created, u.data_size,
            u.data_md5, u.mds_namespace, u.mds_couple_id, u.mds_key_version, u.mds_key_uuid,
            u.storage_class, u.metadata)::v1_code.object_part as part
    ), delete_queue as (
        SELECT part, v1_impl.storage_delete_queue_push(part)
        FROM parts
    ), upload AS(
        SELECT ((p.part).bid, (p.part).cid, (p.part).name, (p.part).object_created, 0 /* part_id */,
           min((p.part).created) FILTER(WHERE (p.part).part_id=0),
           coalesce(sum((p.part).data_size) FILTER(WHERE (p.part).part_id>0), 0),
           (array_agg((p.part).data_md5) FILTER(WHERE (p.part).part_id=0))[1],
           min((p.part).mds_namespace) FILTER(WHERE (p.part).part_id=0),
           min((p.part).mds_couple_id) FILTER(WHERE (p.part).part_id=0),
           min((p.part).mds_key_version) FILTER(WHERE (p.part).part_id=0),
           (array_agg((p.part).mds_key_uuid) FILTER(WHERE (p.part).part_id=0))[1],
           min((p.part).storage_class) FILTER(WHERE (p.part).part_id=0),
           null
        )::v1_code.object_part as part
        FROM delete_queue p
        GROUP BY (p.part).bid, (p.part).cid, (p.part).name, (p.part).object_created
    ), counters as (
        SELECT root_part.part,
            v1_code.chunks_counters_queue_push(OPERATOR(v1_code.-) v1_code.active_multipart_get_chunk_counters(
                        (root_part.part).bid,
                        (root_part.part).cid,
                        (root_part.part).storage_class,
                        count(parts.part),
                        coalesce(sum((parts.part).data_size), 0)::bigint
                      ))
        FROM upload root_part
            LEFT JOIN delete_queue parts ON (root_part.part).name = (parts.part).name
                AND (root_part.part).object_created = (parts.part).object_created
                AND (parts.part).part_id > 0
        GROUP BY (root_part.part)
    )
    SELECT (part).name, (part).object_created, NULL::text as error
    FROM counters;
END;
$$;

/*
 * Increase remove_after_ts of a specific deleted object by 'i_delay'.
 *
 * Args:
 * - i_bid:
 *     ID of the bucket where the deleted object lies.
 * - i_name:
 *     Name of the deleted object.
 * - i_part_id:
 *     NULL if the deleted object previously was a simple object or
 *     part ID if the deleted object previously was an object part.
 * - i_mds_namespace:
 *     mds_namespace of the deleted object.
 * - i_mds_couple_id:
 *     ID of MDS couple, where the deleted object lies.
 * - i_mds_key_version:
 *     mds_key_version of the deleted object.
 * - i_mds_key_uuid:
 *     mds_key_uuid of the deleted object.
 * - i_delay:
 *     Increase remove_after_ts by i_delay. Note that
 *     i_delay must be a positive interval lesser than 2 weeks.
 *
 *     Be cautious! Using intervals sometimes will lead to
 *     unexpected results, e.g.:
 *     - not always '1 day'::interval == '24 hours'::interval
 *     - not always current_time_stamp + interval1 + interval2 ==
 *       current_timestamp + interval2 + interval1
 *
 * Returns:
 * - Nothing
 *
 * Raises:
 * - S3D01 (NoSuchDeletedObject):
 *    If the specified deleted object doesn't exist.
 * - 22004 (Invalid argument, i_delay: ``i_delay``):
 *    If the ``i_delay`` is a negative interval or
 *    ``i_delay`` is greater than 2 weeks.
 */
CREATE OR REPLACE FUNCTION v1_code.delay_deletion(
    i_bid uuid,
    i_name text,
    i_part_id integer,
    i_mds_namespace text,
    i_mds_couple_id integer,
    i_mds_key_version integer,
    i_mds_key_uuid uuid,
    i_delay interval
) RETURNS void
LANGUAGE plpgsql
SECURITY DEFINER AS $$
BEGIN
    IF
      i_delay < '0 week'::interval
      or
      i_delay > '2 weeks'::interval
      THEN
        RAISE EXCEPTION 'Invalid argument, i_delay: %', i_delay
            USING ERRCODE = '22004';
    END IF;

    UPDATE s3.storage_delete_queue
        SET remove_after_ts = remove_after_ts + i_delay
        WHERE bid = i_bid
          AND name = i_name
          AND coalesce(part_id, 0) = coalesce(i_part_id, 0)
          AND mds_namespace IS NOT DISTINCT FROM i_mds_namespace
          AND mds_couple_id = i_mds_couple_id
          AND mds_key_version = i_mds_key_version
          AND mds_key_uuid = i_mds_key_uuid;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'No such deleted object version in storage delete queue'
            USING ERRCODE = 'S3D01';
    END IF;
END;
$$;

CREATE OR REPLACE FUNCTION v1_code.delay_deletion(
    i_bid uuid,
    i_name text,
    i_part_id integer,
    i_mds_couple_id integer,
    i_mds_key_version integer,
    i_mds_key_uuid uuid,
    i_delay interval
) RETURNS void
LANGUAGE plpgsql
SECURITY DEFINER AS $$
BEGIN
    PERFORM v1_code.delay_deletion(
        i_bid, i_name, i_part_id, /*i_mds_namespace*/ NULL,
        i_mds_couple_id, i_mds_key_version, i_mds_key_uuid,
        i_delay);
END;
$$;

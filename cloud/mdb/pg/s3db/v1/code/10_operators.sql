CREATE OR REPLACE FUNCTION v1_code._chunk_counters_compatible(
    i_left v1_code.chunk_counters,
    i_right v1_code.chunk_counters
) RETURNS boolean
LANGUAGE plpgsql IMMUTABLE AS $function$
BEGIN
    /*
     * Check that least one has bid and cid IS NOT NULL
     */
    IF coalesce(i_left.bid, i_right.bid) IS NULL OR
        coalesce(i_left.cid, i_right.cid) IS NULL THEN
        RAISE EXCEPTION 'Chunks %, % does not have correct bid or cid',
            i_left, i_right;
    END IF;

    /*
     * Check that chunks have equal bid and cid.
     * NOTE: One chunk allowed to have bid or/and cid IS NULL.
     */
    IF i_left.bid <> i_right.bid
        OR i_left.cid <> i_right.cid THEN
        RAISE EXCEPTION 'Chunks %, % are not equal',
            i_left, i_right;
    END IF;

    IF (i_left IS NOT NULL) AND (i_right IS NOT NULL)
            AND (coalesce(i_left.storage_class, 0) != coalesce(i_right.storage_class, 0)) THEN
        RAISE EXCEPTION 'Chunks %, % has different storage_class',
            i_left, i_right;
    END IF;

    RETURN true;
END
$function$;

CREATE OR REPLACE FUNCTION v1_code._chunk_counters_sum(
    i_left v1_code.chunk_counters,
    i_right v1_code.chunk_counters
) RETURNS v1_code.chunk_counters
LANGUAGE plpgsql IMMUTABLE AS $function$
DECLARE
    v_chunk_counters v1_code.chunk_counters;
BEGIN
    PERFORM v1_code._chunk_counters_compatible(i_left, i_right);

    v_chunk_counters.bid := coalesce(i_left.bid, i_right.bid);
    v_chunk_counters.cid := coalesce(i_left.cid, i_right.cid);
    v_chunk_counters.simple_objects_count := coalesce(i_left.simple_objects_count, 0)
        + coalesce(i_right.simple_objects_count, 0);
    v_chunk_counters.simple_objects_size := coalesce(i_left.simple_objects_size, 0)
        + coalesce(i_right.simple_objects_size, 0);
    v_chunk_counters.multipart_objects_count := coalesce(i_left.multipart_objects_count, 0)
        + coalesce(i_right.multipart_objects_count, 0);
    v_chunk_counters.multipart_objects_size := coalesce(i_left.multipart_objects_size, 0)
        + coalesce(i_right.multipart_objects_size, 0);
    v_chunk_counters.objects_parts_count := coalesce(i_left.objects_parts_count, 0)
        + coalesce(i_right.objects_parts_count, 0);
    v_chunk_counters.objects_parts_size := coalesce(i_left.objects_parts_size, 0)
        + coalesce(i_right.objects_parts_size, 0);
    v_chunk_counters.deleted_objects_count := coalesce(i_left.deleted_objects_count, 0)
        + coalesce(i_right.deleted_objects_count, 0);
    v_chunk_counters.deleted_objects_size := coalesce(i_left.deleted_objects_size, 0)
        + coalesce(i_right.deleted_objects_size, 0);
    v_chunk_counters.active_multipart_count := coalesce(i_left.active_multipart_count, 0)
        + coalesce(i_right.active_multipart_count, 0);

    IF (i_left.storage_class IS NOT NULL) THEN
      v_chunk_counters.storage_class := i_left.storage_class;
    ELSE
      v_chunk_counters.storage_class := coalesce(i_right.storage_class, 0);
    END IF;

    RETURN v_chunk_counters;
END
$function$;

CREATE OPERATOR v1_code.+ (
    PROCEDURE = v1_code._chunk_counters_sum,
    LEFTARG = v1_code.chunk_counters,
    RIGHTARG = v1_code.chunk_counters
);

CREATE OR REPLACE FUNCTION v1_code._chunk_counters_minus(
    i_left v1_code.chunk_counters,
    i_right v1_code.chunk_counters
) RETURNS v1_code.chunk_counters
LANGUAGE plpgsql IMMUTABLE AS $function$
DECLARE
    v_chunk_counters v1_code.chunk_counters;
BEGIN
    PERFORM v1_code._chunk_counters_compatible(i_left, i_right);

    v_chunk_counters.bid := coalesce(i_left.bid, i_right.bid);
    v_chunk_counters.cid := coalesce(i_left.cid, i_right.cid);
    v_chunk_counters.simple_objects_count := coalesce(i_left.simple_objects_count, 0)
        - coalesce(i_right.simple_objects_count, 0);
    v_chunk_counters.simple_objects_size := coalesce(i_left.simple_objects_size, 0)
        - coalesce(i_right.simple_objects_size, 0);
    v_chunk_counters.multipart_objects_count := coalesce(i_left.multipart_objects_count, 0)
        - coalesce(i_right.multipart_objects_count, 0);
    v_chunk_counters.multipart_objects_size := coalesce(i_left.multipart_objects_size, 0)
        - coalesce(i_right.multipart_objects_size, 0);
    v_chunk_counters.objects_parts_count := coalesce(i_left.objects_parts_count, 0)
        - coalesce(i_right.objects_parts_count, 0);
    v_chunk_counters.objects_parts_size := coalesce(i_left.objects_parts_size, 0)
        - coalesce(i_right.objects_parts_size, 0);
    v_chunk_counters.deleted_objects_count := coalesce(i_left.deleted_objects_count, 0)
        - coalesce(i_right.deleted_objects_count, 0);
    v_chunk_counters.deleted_objects_size := coalesce(i_left.deleted_objects_size, 0)
        - coalesce(i_right.deleted_objects_size, 0);
    v_chunk_counters.active_multipart_count := coalesce(i_left.active_multipart_count, 0)
        - coalesce(i_right.active_multipart_count, 0);

    IF (i_left.storage_class IS NOT NULL) THEN
      v_chunk_counters.storage_class := i_left.storage_class;
    ELSE
      v_chunk_counters.storage_class := coalesce(i_right.storage_class, 0);
    END IF;

    RETURN v_chunk_counters;
END
$function$;

CREATE OPERATOR v1_code.- (
    PROCEDURE = v1_code._chunk_counters_minus,
    LEFTARG = v1_code.chunk_counters,
    RIGHTARG = v1_code.chunk_counters
);

CREATE OR REPLACE FUNCTION v1_code._chunk_counters_negative(
    i_counters v1_code.chunk_counters
) RETURNS v1_code.chunk_counters
LANGUAGE plpgsql IMMUTABLE AS $function$
DECLARE
    v_result v1_code.chunk_counters;
BEGIN
    v_result := i_counters;
    v_result.simple_objects_count = -1 * i_counters.simple_objects_count;
    v_result.simple_objects_size = -1 * i_counters.simple_objects_size;
    v_result.multipart_objects_count = -1 * i_counters.multipart_objects_count;
    v_result.multipart_objects_size = -1 * i_counters.multipart_objects_size;
    v_result.objects_parts_count = -1 * i_counters.objects_parts_count;
    v_result.objects_parts_size = -1 * i_counters.objects_parts_size;
    v_result.deleted_objects_count = -1 * i_counters.deleted_objects_count;
    v_result.deleted_objects_size = -1 * i_counters.deleted_objects_size;
    v_result.active_multipart_count = -1 * i_counters.active_multipart_count;

    RETURN v_result;
END
$function$;

CREATE OPERATOR v1_code.- (
    PROCEDURE = v1_code._chunk_counters_negative,
    RIGHTARG = v1_code.chunk_counters
);


/*
 * Returns counters change for object's chunk
 *
 * Args:
 * - i_object:
 *     Object.
 *
 * Returns:
 *   A ``code.chunk_counters`` instances that represents the counters
 *   change for object's chunk.
 */
CREATE OR REPLACE FUNCTION v1_code.object_get_chunk_counters(
    i_object v1_code.object
) RETURNS v1_code.chunk_counters
LANGUAGE plpgsql IMMUTABLE AS $function$
DECLARE
    v_chunk_counters v1_code.chunk_counters;
BEGIN
    SELECT i_object.bid, i_object.cid,
        0 /* simple_objects_count */,
        0 /* simple_objects_size */,
        0 /* multipart_objects_count */,
        0 /* multipart_objects_size */,
        0 /* objects_parts_count */,
        0 /* objects_parts_size */,
        0 /* deleted_objects_count */,
        0 /* deleted_objects_size */,
        0 /* active_multipart_count */,
        i_object.storage_class
        INTO v_chunk_counters;

    IF coalesce(i_object.parts_count, 0) = 0 THEN
        v_chunk_counters.simple_objects_count := 1;
        IF i_object.delete_marker THEN
            v_chunk_counters.simple_objects_size := length(i_object.name);
        ELSE
            v_chunk_counters.simple_objects_size := i_object.data_size;
        END IF;
    ELSE
        v_chunk_counters.multipart_objects_count := 1;
        v_chunk_counters.multipart_objects_size := i_object.data_size;
    END IF;

    RETURN v_chunk_counters;
END;
$function$;

CREATE OR REPLACE FUNCTION v1_code._chunk_counters_sum_object(
    i_chunk_counters v1_code.chunk_counters,
    i_object v1_code.object
) RETURNS v1_code.chunk_counters
LANGUAGE sql IMMUTABLE AS $function$
    SELECT i_chunk_counters OPERATOR(v1_code.+) v1_code.object_get_chunk_counters(i_object);
$function$;

CREATE OPERATOR v1_code.+ (
    PROCEDURE = v1_code._chunk_counters_sum_object,
    LEFTARG = v1_code.chunk_counters,
    RIGHTARG = v1_code.object
);

CREATE OR REPLACE FUNCTION v1_code._chunk_counters_minus_object(
    i_chunk_counters v1_code.chunk_counters,
    i_object v1_code.object
) RETURNS v1_code.chunk_counters
LANGUAGE sql IMMUTABLE AS $function$
    SELECT i_chunk_counters OPERATOR(v1_code.-) v1_code.object_get_chunk_counters(i_object);
$function$;

CREATE OPERATOR v1_code.- (
    PROCEDURE = v1_code._chunk_counters_minus_object,
    LEFTARG = v1_code.chunk_counters,
    RIGHTARG = v1_code.object
);

/*
 * Returns counters change for object delete marker's chunk
 *
 * Args:
 * - i_marker:
 *     Object delete marker.
 *
 * Returns:
 *   A ``code.chunk_counters`` instances that represents the counters
 *   change for object delete marker's chunk.
 */
CREATE OR REPLACE FUNCTION v1_code.delete_marker_get_chunk_counters(
    i_cid bigint, i_marker v1_code.object_delete_marker
) RETURNS v1_code.chunk_counters
LANGUAGE plpgsql IMMUTABLE AS $function$
DECLARE
    v_chunk_counters v1_code.chunk_counters;
BEGIN
    SELECT i_marker.bid, i_cid,
        1 /* simple_objects_count */,
        length(i_marker.name) /* simple_objects_size */,
        0 /* multipart_objects_count */,
        0 /* multipart_objects_size */,
        0 /* objects_parts_count */,
        0 /* objects_parts_size */,
        0 /* deleted_objects_count */,
        0 /* deleted_objects_size */,
        0 /* active_multipart_count */,
        0 /* storage_class */
        INTO v_chunk_counters;

    RETURN v_chunk_counters;
END;
$function$;

/*
 * Returns counters change for part's chunk
 *
 * Args:
 * - i_part: object part.
 *
 * Returns:
 *   A ``code.chunk_counters`` instances that represents the counters
 *   change for part's chunk.
 */
CREATE OR REPLACE FUNCTION v1_code.object_part_get_chunk_counters(
    i_part v1_code.object_part
) RETURNS v1_code.chunk_counters
LANGUAGE plpgsql IMMUTABLE AS $function$
DECLARE
    v_chunk_counters v1_code.chunk_counters;
BEGIN
    SELECT i_part.bid, i_part.cid,
        0 /* simple_objects_count */,
        0 /* simple_objects_size */,
        0 /* multipart_objects_count */,
        0 /* multipart_objects_size */,
        0 /* objects_parts_count */,
        0 /* objects_parts_size */,
        0 /* deleted_objects_count */,
        0 /* deleted_objects_size */,
        0 /* active_multipart_count */,
        i_part.storage_class
        INTO v_chunk_counters;

    v_chunk_counters.objects_parts_count := 1;
    v_chunk_counters.objects_parts_size := i_part.data_size;

    RETURN v_chunk_counters;
END;
$function$;

/*
 * Returns counters change for aborted/completed multipart chunk
 */
CREATE OR REPLACE FUNCTION v1_code.active_multipart_get_chunk_counters(
    i_bid uuid, i_cid bigint, i_storage_class int, i_parts_count bigint, i_parts_size bigint
) RETURNS v1_code.chunk_counters
LANGUAGE plpgsql IMMUTABLE AS $function$
DECLARE
    v_chunk_counters v1_code.chunk_counters;
BEGIN
    SELECT i_bid, i_cid,
        0 /* simple_objects_count */,
        0 /* simple_objects_size */,
        0 /* multipart_objects_count */,
        0 /* multipart_objects_size */,
        i_parts_count /* objects_parts_count */,
        i_parts_size /* objects_parts_size */,
        0 /* deleted_objects_count */,
        0 /* deleted_objects_size */,
        1 /* active_multipart_count */,
        i_storage_class
        INTO v_chunk_counters;

    RETURN v_chunk_counters;
END;
$function$;

CREATE OR REPLACE FUNCTION v1_code._chunk_counters_sum_object_part(
    i_chunk_counters v1_code.chunk_counters,
    i_part v1_code.object_part
) RETURNS v1_code.chunk_counters
LANGUAGE sql IMMUTABLE AS $function$
    SELECT i_chunk_counters OPERATOR(v1_code.+) v1_code.object_part_get_chunk_counters(i_part);
$function$;

CREATE OPERATOR v1_code.+ (
    PROCEDURE = v1_code._chunk_counters_sum_object_part,
    LEFTARG = v1_code.chunk_counters,
    RIGHTARG = v1_code.object_part
);

CREATE OR REPLACE FUNCTION v1_code._chunk_counters_minus_object_part(
    i_chunk_counters v1_code.chunk_counters,
    i_object_part v1_code.object_part
) RETURNS v1_code.chunk_counters
LANGUAGE sql IMMUTABLE AS $function$
    SELECT i_chunk_counters OPERATOR(v1_code.-) v1_code.object_part_get_chunk_counters(i_object_part);
$function$;

CREATE OPERATOR v1_code.- (
    PROCEDURE = v1_code._chunk_counters_minus_object_part,
    LEFTARG = v1_code.chunk_counters,
    RIGHTARG = v1_code.object_part
);


/*
 * Returns counters change for deleted object
 *
 * Args:
 * - i_deleted_object:
 *     v1_code.deleted_object
 *
 * Returns:
 *   A ``code.chunk_counters`` instances that represents the counters
 *   change for object's chunk.
 */

CREATE OR REPLACE FUNCTION v1_code.object_deleted_get_chunk_counters(
    i_deleted_object v1_code.deleted_object
) RETURNS v1_code.chunk_counters
LANGUAGE plpgsql IMMUTABLE AS $function$
DECLARE
    v_chunk_counters v1_code.chunk_counters;
BEGIN
    SELECT i_deleted_object.bid,
        NULL,
        0 /* simple_objects_count */,
        0 /* simple_objects_size */,
        0 /* multipart_objects_count */,
        0 /* multipart_objects_size */,
        0 /* objects_parts_count */,
        0 /* objects_parts_size */,
        1 /* deleted_objects_count */,
        i_deleted_object.data_size,
        0 /* active_multipart_count */,
        i_deleted_object.storage_class
        INTO v_chunk_counters;

    v_chunk_counters.cid := v1_code.get_object_cid_non_blocking(i_deleted_object.bid, i_deleted_object.name, FALSE);

    RETURN v_chunk_counters;
END;
$function$;

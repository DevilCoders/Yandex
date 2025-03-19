CREATE OR REPLACE FUNCTION v1_impl.get_object_part_listing_item(
    i_obj_part s3.object_parts,
    i_prefix text,
    i_delimiter text,
    i_level int
)
RETURNS v1_impl.object_part_listing_item
LANGUAGE plpgsql STABLE AS $function$
DECLARE
    v_result v1_impl.object_part_listing_item;
    v_common_prefix text;
    v_obj_part s3.object_parts;
BEGIN
    v_common_prefix := v1_impl.get_key_common_prefix(i_obj_part.name, i_prefix, i_delimiter);
    IF v_common_prefix IS NOT NULL THEN
        v_obj_part.name := v_common_prefix;
        SELECT
            v_obj_part,
            v1_impl.get_last_prefix_upload(i_obj_part.bid, v_common_prefix),
            NULL,
            i_level
            INTO v_result;
    ELSE
        SELECT
            i_obj_part,
            i_obj_part.name,
            i_obj_part.object_created,
            i_level
            INTO v_result;
    END IF;
    RETURN v_result;
END
$function$;

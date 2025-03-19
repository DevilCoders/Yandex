CREATE OR REPLACE FUNCTION v1_impl.get_object_listing_item(
    i_obj s3.objects,
    i_prefix text,
    i_delimiter text,
    i_level int
)
RETURNS v1_impl.object_listing_item
LANGUAGE plpgsql STABLE AS $function$
DECLARE
    v_result v1_impl.object_listing_item;
    v_common_prefix text;
    v_obj s3.objects;
BEGIN
    v_common_prefix := v1_impl.get_key_common_prefix(i_obj.name, i_prefix, i_delimiter);
    IF v_common_prefix IS NOT NULL THEN
        v_obj.name := v_common_prefix;
        SELECT v_obj, v1_impl.get_last_prefix_key(i_obj.bid, v_common_prefix), i_level
            INTO v_result;
    ELSE
        SELECT i_obj, i_obj.name, i_level
            INTO v_result;
    END IF;
    RETURN v_result;
END
$function$;

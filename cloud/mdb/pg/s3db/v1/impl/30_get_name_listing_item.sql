CREATE OR REPLACE FUNCTION v1_impl.get_name_listing_item(
    i_bid uuid,
    i_name text,
    i_prefix text,
    i_delimiter text,
    i_after_delimiter text,
    i_level int
)
RETURNS v1_impl.list_name_item
LANGUAGE plpgsql STABLE AS $function$
DECLARE
    v_result v1_impl.list_name_item;
    v_common_prefix text;
BEGIN
    v_common_prefix := v1_impl.get_key_common_prefix(i_name, i_prefix, i_delimiter);
    IF v_common_prefix IS NOT NULL THEN
        SELECT i_bid, v_common_prefix, left(v_common_prefix, char_length(v_common_prefix) - 1) || i_after_delimiter, true, i_level
            INTO v_result;
    ELSE
        SELECT i_bid, i_name, i_name || chr(1), false, i_level
            INTO v_result;
    END IF;
    RETURN v_result;
END
$function$;

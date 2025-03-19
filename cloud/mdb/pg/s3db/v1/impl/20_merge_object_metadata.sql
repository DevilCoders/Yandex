CREATE OR REPLACE FUNCTION v1_impl.merge_object_metadata(
    i_new_meta JSONB,
    i_original_meta JSONB
)
RETURNS JSONB
LANGUAGE plpgsql IMMUTABLE AS $function$
DECLARE
    v_metadata JSONB;
BEGIN
    v_metadata := coalesce(i_new_meta, i_original_meta, '{}'::JSONB);
    IF NOT v_metadata ? 'encryption' THEN
        v_metadata := jsonb_strip_nulls(v_metadata || jsonb_build_object('encryption', i_original_meta #> '{encryption}'));
    END IF;
    RETURN v_metadata;
END
$function$;

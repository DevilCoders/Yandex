CREATE OR REPLACE FUNCTION s3.to_keyrange(start_key text, end_key text)
 RETURNS s3.keyrange
 LANGUAGE plpgsql
 IMMUTABLE
AS $function$
BEGIN
    IF start_key IS NULL AND end_key IS NULL THEN
        RETURN '[,)'::s3.keyrange;
    ELSIF start_key IS NULL THEN
        RETURN format('[,%I)', end_key)::s3.keyrange;
    ELSIF end_key IS NULL THEN
        RETURN format('[%I,)', start_key)::s3.keyrange;
    ELSE
        RETURN format('[%I,%I)', start_key, end_key)::s3.keyrange;
    END IF;
END;
$function$

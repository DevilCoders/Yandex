CREATE OR REPLACE FUNCTION v1_code.list_access_keys()
RETURNS SETOF v1_code.access_key
LANGUAGE sql STABLE AS $function$
    SELECT service_id, user_id, role, key_id, secret_token, key_version, issue_date
    FROM s3.access_keys;
$function$;

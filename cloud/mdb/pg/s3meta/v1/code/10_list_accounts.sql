CREATE OR REPLACE FUNCTION v1_code.list_accounts()
RETURNS SETOF v1_code.account
LANGUAGE sql STABLE AS $function$
    SELECT service_id, status, registration_date, max_size, max_buckets, folder_id, cloud_id
        FROM s3.accounts;
$function$;

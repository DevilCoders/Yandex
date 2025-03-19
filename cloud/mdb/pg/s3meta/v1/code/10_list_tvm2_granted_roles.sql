CREATE OR REPLACE FUNCTION v1_code.list_tvm2_granted_roles()
RETURNS SETOF v1_code.granted_role
LANGUAGE sql STABLE AS $function$
    SELECT service_id, role, grantee_uid, issue_date
        FROM s3.tvm2_granted_roles;
$function$;

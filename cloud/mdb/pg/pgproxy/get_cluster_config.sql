CREATE OR REPLACE FUNCTION plproxy.get_cluster_config(i_cluster_name text, out o_key text, out o_val text)
 RETURNS SETOF record
 LANGUAGE plpgsql
AS $function$
BEGIN
    if i_cluster_name not in ('rw', 'ro') then
        raise exception 'Unknown cluster';
    end if;
    return query select key::text as o_key, value::text as o_val from plproxy.config;
    return;
END;
$function$

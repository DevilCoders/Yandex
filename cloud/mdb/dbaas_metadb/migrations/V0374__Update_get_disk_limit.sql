-- New functions
CREATE OR REPLACE FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type text, i_flavor text)
 RETURNS bigint
 LANGUAGE plpgsql
 IMMUTABLE
AS $function$
DECLARE
    v_disk_type      dbaas.disk_type;
    v_flavor         dbaas.flavors;
BEGIN
    SELECT *
      INTO v_flavor
      FROM dbaas.flavors
     WHERE name = i_flavor;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find flavor by name: %', i_flavor
        USING ERRCODE = 'MDB02';
    END IF;

    SELECT *
      INTO v_disk_type
      FROM dbaas.disk_type
     WHERE disk_type_ext_id = i_disk_type;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find disk_type by disk_type_ext_id: %', i_disk_type
        USING ERRCODE = 'MDB02';
    END IF;

    IF (v_flavor).generation < 4 AND (v_flavor).vtype = 'porto' THEN
        RETURN (v_flavor).io_limit;
    END IF;

    RETURN code.dynamic_io_limit(i_space_limit, v_disk_type, (v_flavor).io_limit);
END;
$function$;
DROP FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type dbaas.disk_type, i_flavor dbaas.flavors);

GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type text, i_flavor text) TO backup_cli;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type text, i_flavor text) TO cms;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type text, i_flavor text) TO dbaas_api;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type text, i_flavor text) TO dbaas_support;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type text, i_flavor text) TO dbaas_worker;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type text, i_flavor text) TO idm_service;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type text, i_flavor text) TO katan_imp;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type text, i_flavor text) TO logs_api;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type text, i_flavor text) TO mdb_downtimer;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type text, i_flavor text) TO mdb_health;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type text, i_flavor text) TO mdb_maintenance;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type text, i_flavor text) TO mdb_ui;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type text, i_flavor text) TO pillar_config;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type text, i_flavor text) TO pillar_secrets;

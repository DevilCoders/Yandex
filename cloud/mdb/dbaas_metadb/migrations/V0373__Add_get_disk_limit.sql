CREATE OR REPLACE FUNCTION code.get_disk_limit(
    i_space_limit bigint,
    i_disk_type   dbaas.disk_type,
    i_flavor      dbaas.flavors
) RETURNS bigint AS $$
DECLARE
BEGIN
    IF (i_flavor).generation < 4 AND (i_flavor).vtype = 'porto' THEN
        RETURN (i_flavor).io_limit;
    END IF;

    RETURN code.dynamic_io_limit(i_space_limit, i_disk_type, (i_flavor).io_limit);
END;
$$ LANGUAGE plpgsql IMMUTABLE;

GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type dbaas.disk_type, i_flavor dbaas.flavors) TO backup_cli;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type dbaas.disk_type, i_flavor dbaas.flavors) TO cms;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type dbaas.disk_type, i_flavor dbaas.flavors) TO dbaas_api;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type dbaas.disk_type, i_flavor dbaas.flavors) TO dbaas_support;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type dbaas.disk_type, i_flavor dbaas.flavors) TO dbaas_worker;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type dbaas.disk_type, i_flavor dbaas.flavors) TO idm_service;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type dbaas.disk_type, i_flavor dbaas.flavors) TO katan_imp;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type dbaas.disk_type, i_flavor dbaas.flavors) TO logs_api;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type dbaas.disk_type, i_flavor dbaas.flavors) TO mdb_downtimer;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type dbaas.disk_type, i_flavor dbaas.flavors) TO mdb_health;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type dbaas.disk_type, i_flavor dbaas.flavors) TO mdb_maintenance;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type dbaas.disk_type, i_flavor dbaas.flavors) TO mdb_ui;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type dbaas.disk_type, i_flavor dbaas.flavors) TO pillar_config;
GRANT ALL ON FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type dbaas.disk_type, i_flavor dbaas.flavors) TO pillar_secrets;

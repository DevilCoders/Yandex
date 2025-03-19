-- Changed functions
CREATE OR REPLACE FUNCTION code.get_disk_limit(i_space_limit bigint, i_disk_type text, i_flavor text)
 RETURNS bigint
 LANGUAGE plpgsql
 STABLE
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
        RAISE EXCEPTION 'Unknown resource_preset: %', i_flavor
        USING ERRCODE = 'MDB02';
    END IF;

    SELECT *
      INTO v_disk_type
      FROM dbaas.disk_type
     WHERE disk_type_ext_id = i_disk_type;

    IF NOT found THEN
        RAISE EXCEPTION 'Unknown disk_type: %', i_disk_type
        USING ERRCODE = 'MDB02';
    END IF;

    IF (v_flavor).generation < 4 AND (v_flavor).vtype = 'porto' THEN
        RETURN (v_flavor).io_limit;
    END IF;

    RETURN code.dynamic_io_limit(i_space_limit, v_disk_type, (v_flavor).io_limit);
END;
$function$;

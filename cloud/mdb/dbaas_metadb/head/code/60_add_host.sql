CREATE OR REPLACE FUNCTION code.add_host(
    i_subcid           text,
    i_shard_id         text,
    i_space_limit      bigint,
    i_flavor_id        uuid,
    i_geo              text,
    i_fqdn             text,
    i_disk_type        text,
    i_subnet_id        text,
    i_assign_public_ip boolean,
    i_cid              text,
    i_rev              bigint
) RETURNS SETOF code.host AS $$
DECLARE
    v_host      dbaas.hosts;
    v_geo       dbaas.geo;
    v_disk_type dbaas.disk_type;
    v_flavor    dbaas.flavors;
BEGIN
    SELECT *
      INTO v_geo
      FROM dbaas.geo
     WHERE name = i_geo;

    IF NOT found THEN
        RAISE EXCEPTION 'Can''t find geo_id for geo: %', i_geo
              USING TABLE = 'dbaas.geo';
    END IF;

    SELECT *
    INTO v_disk_type
    FROM dbaas.disk_type
    WHERE disk_type_ext_id = i_disk_type;

    IF NOT found THEN
        RAISE EXCEPTION
            'Can''t find disk_type_id for disk_type_ext_id: %', i_disk_type
            USING TABLE = 'dbaas.disk_type';
    END IF;

    SELECT *
    INTO v_flavor
    FROM dbaas.flavors
    WHERE id = i_flavor_id;

    IF NOT found THEN
        RAISE EXCEPTION
            'Can''t find flavor for id: %', i_flavor_id
            USING TABLE = 'dbaas.flavors';
    END IF;

    INSERT INTO dbaas.hosts
        (subcid, shard_id, space_limit,
        flavor, geo_id, fqdn, disk_type_id,
        subnet_id, assign_public_ip)
    VALUES
        (i_subcid, i_shard_id, i_space_limit,
        i_flavor_id, (v_geo).geo_id, i_fqdn,
        (v_disk_type).disk_type_id, i_subnet_id,
        i_assign_public_ip)
    RETURNING * INTO v_host;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'add_host',
            jsonb_build_object(
                'fqdn', i_fqdn
            )
        )
    );

    RETURN QUERY
        SELECT fmt.*
          FROM dbaas.subclusters s,
               dbaas.clusters c,
               code.format_host(v_host, c, s, v_geo, v_disk_type, v_flavor) fmt
         WHERE s.subcid = (v_host).subcid
           AND c.cid = s.cid;
END;
$$ LANGUAGE plpgsql;

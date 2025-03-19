CREATE OR REPLACE FUNCTION code.update_host(
    i_fqdn              text,
    i_cid               text,
    i_rev               bigint,
    i_space_limit       bigint DEFAULT NULL,
    i_flavor_id         uuid   DEFAULT NULL,
    i_vtype_id          text   DEFAULT NULL,
    i_disk_type         text   DEFAULT NULL,
    i_assign_public_ip  bool   DEFAULT NULL,
    i_subnet_id         text   DEFAULT NULL
) RETURNS SETOF code.host AS $$
DECLARE
    v_disk_type dbaas.disk_type;
BEGIN
    IF i_disk_type IS NOT NULL THEN
        SELECT *
        INTO v_disk_type
        FROM dbaas.disk_type
        WHERE disk_type_ext_id = i_disk_type;

        IF NOT found THEN
            RAISE EXCEPTION
                'Can''t find disk_type_id for disk_type_ext_id: %', i_disk_type
                USING TABLE = 'dbaas.disk_type';
        END IF;
    END IF;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'update_host',
            jsonb_build_object(
                'fqdn', i_fqdn
            )
        )
    );

RETURN QUERY
    WITH host_updated AS (
        UPDATE dbaas.hosts
           SET space_limit = coalesce(i_space_limit, space_limit),
               flavor = coalesce(i_flavor_id, flavor),
               vtype_id = coalesce(i_vtype_id, vtype_id),
               disk_type_id = coalesce((v_disk_type).disk_type_id, disk_type_id),
               assign_public_ip = coalesce(i_assign_public_ip, assign_public_ip),
               subnet_id = coalesce(i_subnet_id, subnet_id)
         WHERE fqdn = i_fqdn
        RETURNING *)
    SELECT fmt.* 
      FROM host_updated h
      JOIN dbaas.subclusters s
     USING (subcid)
      JOIN dbaas.clusters c
     USING (cid)
      JOIN dbaas.geo g
     USING (geo_id)
      JOIN dbaas.disk_type d
     USING (disk_type_id)
      JOIN dbaas.flavors f
        ON (h.flavor = f.id),
           code.format_host(h, c, s, g, d, f) fmt;
END;
$$ LANGUAGE plpgsql;

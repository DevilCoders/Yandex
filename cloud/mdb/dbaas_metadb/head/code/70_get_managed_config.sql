CREATE OR REPLACE FUNCTION code.dynamic_io_limit(
    i_space_limit     bigint,
    i_disk_type       dbaas.disk_type,
    i_static_io_limit bigint
) RETURNS bigint AS $$
SELECT COALESCE(
    LEAST(
        CEIL(i_space_limit / (i_disk_type).allocation_unit_size)::bigint * (i_disk_type).io_limit_per_allocation_unit,
        (i_disk_type).io_limit_max
        ),
    i_static_io_limit
    );
$$ LANGUAGE SQL IMMUTABLE;


CREATE OR REPLACE FUNCTION code.get_managed_config(
    i_fqdn      text,
    i_target_id text   DEFAULT NULL,
    i_rev       bigint DEFAULT NULL
) RETURNS jsonb AS $$
DECLARE
    v_rev            bigint;
    v_cluster        dbaas.clusters;
    v_config         jsonb[];
    v_custom_pillar  jsonb;
    v_derived_config jsonb;
    v_backup_schedule jsonb;
    v_host           dbaas.hosts;
    v_disk_type      dbaas.disk_type;
    v_flavor         dbaas.flavors;
    v_subcluster     dbaas.subclusters;
    v_shard          dbaas.shards;
    v_folder         dbaas.folders;
    v_cloud          dbaas.clouds;
    v_geo            dbaas.geo;
    v_region         dbaas.regions;
    component        dbaas.role_type;
    host             code.config_host;
    v_version        code.version;
    v_cluster_hosts  jsonb;
BEGIN
    IF i_rev IS NULL THEN
        SELECT c.actual_rev
          INTO v_rev
          FROM dbaas.clusters c
               JOIN dbaas.subclusters sc USING (cid)
               JOIN dbaas.hosts h USING (subcid)
         WHERE h.fqdn = i_fqdn
               AND code.visible(c)
               AND code.managed(c);

        IF NOT found THEN
            RAISE EXCEPTION 'Unable to find managed cluster by host %', i_fqdn
            USING ERRCODE = 'MDB01';
        END IF;
    ELSE
        v_rev := i_rev;
    END IF;

    SELECT c.cid,
           cr.name,
           c.type,
           c.env,
           c.created_at,
           c.public_key,
           cr.network_id,
           cr.folder_id,
           cr.description,
           cr.status,
           v_rev AS actual_rev,
           c.next_rev,
           cr.host_group_ids,
           c.deletion_protection,
           c.monitoring_cloud_id
      INTO v_cluster
      FROM dbaas.clusters c
           JOIN dbaas.clusters_revs cr ON (c.cid = cr.cid)
           JOIN dbaas.subclusters_revs sc ON (c.cid = sc.cid)
           JOIN dbaas.hosts_revs h USING (subcid)
     WHERE cr.rev = v_rev
           AND h.rev = v_rev
           AND sc.rev = v_rev
           AND h.fqdn = i_fqdn
           AND code.visible(c)
           AND code.managed(c);

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find managed cluster by host %', i_fqdn
        USING ERRCODE = 'MDB01';
    END IF;

    v_config := ARRAY(SELECT value FROM code.get_rev_pillar_by_host(i_fqdn, v_rev, i_target_id) ORDER BY priority);

    v_custom_pillar := code.get_rev_custom_pillar_by_host(i_fqdn, (v_cluster).type, v_rev);

    SELECT subcid,
           shard_id,
           flavor,
           space_limit,
           fqdn,
           vtype_id,
           geo_id,
           disk_type_id,
           subnet_id,
           assign_public_ip,
           created_at,
           is_in_user_project_id
      INTO v_host
      FROM dbaas.hosts_revs
     WHERE rev = v_rev
       AND fqdn = i_fqdn;

    SELECT *
      INTO v_flavor
      FROM dbaas.flavors
     WHERE id = (v_host).flavor;

    SELECT *
      INTO v_disk_type
      FROM dbaas.disk_type
     WHERE disk_type_id = (v_host).disk_type_id;

    SELECT subcid,
           cid,
           name,
           roles,
           created_at
      INTO v_subcluster
      FROM dbaas.subclusters_revs
     WHERE rev = v_rev
       AND subcid = (v_host).subcid;

    SELECT subcid,
           shard_id,
           name,
           created_at
      INTO v_shard
      FROM dbaas.shards_revs
     WHERE rev = v_rev
       AND shard_id = (v_host).shard_id;

    SELECT *
      INTO v_folder
      FROM dbaas.folders
     WHERE folder_id = (v_cluster).folder_id;

    SELECT *
      INTO v_cloud
      FROM dbaas.clouds
     WHERE cloud_id = (v_folder).cloud_id;

    SELECT *
      INTO v_geo
      FROM dbaas.geo
     WHERE geo_id = (v_host).geo_id;

    SELECT *
      INTO v_region
      FROM dbaas.regions
     WHERE region_id = (v_geo).region_id;

    SELECT coalesce(schedule, '{}'::jsonb)
      INTO v_backup_schedule
      FROM dbaas.backup_schedule
     WHERE cid = (v_cluster).cid;

    WITH cte AS (
        SELECT
            h.fqdn,
            fld.folder_ext_id folder_id,
            cld.cloud_ext_id cloud_id,
            f.name resource_preset_id,
            f.platform_id platform_id,
            f.cpu_limit::numeric cores,
            (f.cpu_guarantee * 100 / f.cpu_limit)::int core_fraction,
            f.gpu_limit gpu_limit,
            f.memory_limit memory,
            f.io_cores_limit io_cores_limit,
            cv.cid cluster_id,
            c.type cluster_type,
            dt.disk_type_ext_id disk_type_id,
            h.space_limit disk_size,
            h.assign_public_ip assign_public_ip,
            sc.roles roles,
            h.vtype_id compute_instance_id,
            coalesce(cardinality(cv.host_group_ids), 0) > 0 on_dedicated_host
        FROM
                dbaas.clusters c
            INNER JOIN
                dbaas.clusters_revs cv
                    ON cv.cid = c.cid AND cv.rev = v_rev
            INNER JOIN
                dbaas.subclusters_revs sc
                    ON c.cid = sc.cid AND sc.rev = v_rev
            INNER JOIN
                dbaas.hosts_revs h
                    ON h.subcid = sc.subcid AND h.rev = v_rev
            INNER JOIN
                dbaas.flavors f
                    ON f.id = h.flavor
            INNER JOIN
                dbaas.folders fld
                    ON fld.folder_id = cv.folder_id
            INNER JOIN
                dbaas.clouds cld
                    ON cld.cloud_id = fld.cloud_id
            INNER JOIN
                dbaas.disk_type dt
                    ON dt.disk_type_id = h.disk_type_id
            WHERE c.cid = (v_cluster).cid
    )
    SELECT jsonb_object_agg(r.fqdn, to_jsonb(r.*))
        INTO v_cluster_hosts
        FROM cte r;

    v_derived_config := jsonb_build_object(
        'data', jsonb_build_object(
            'backup', v_backup_schedule,
            'runlist', jsonb_build_array(),
            'dbaas', jsonb_build_object(
                'fqdn', i_fqdn,
                'cluster_id', (v_cluster).cid,
                'cluster_name', (v_cluster).name,
                'cluster_type', (v_cluster).type,
                'cluster', jsonb_build_object(
                    'subclusters', jsonb_build_object()
                ),
                'subcluster_id', (v_subcluster).subcid,
                'subcluster_name', (v_subcluster).name,
                'shard_id', (v_host).shard_id,
                'shard_name', (CASE WHEN (v_host).shard_id IS NOT NULL THEN (v_shard).name ELSE NULL END),
                'vtype', (v_flavor).vtype,
                'vtype_id', (v_host).vtype_id,
                'shard_hosts', jsonb_build_array(),
                'cluster_hosts', jsonb_build_array(),
                'folder', jsonb_build_object(
                    'folder_ext_id', (v_folder).folder_ext_id
                ),
                'cloud', jsonb_build_object(
                    'cloud_ext_id', (v_cloud).cloud_ext_id
                ),
                'flavor', jsonb_build_object(
                    'id', (v_flavor).id,
                    'cpu_guarantee', to_jsonb(1.0 * (v_flavor).cpu_guarantee::numeric),
                    'cpu_limit', to_jsonb(1.0 * (v_flavor).cpu_limit::numeric),
                    'cpu_fraction', ((v_flavor).cpu_guarantee * 100 / (v_flavor).cpu_limit)::int,
                    'gpu_limit', (v_flavor).gpu_limit,
                    'memory_guarantee', (v_flavor).memory_guarantee,
                    'memory_limit', (v_flavor).memory_limit,
                    'network_guarantee', (v_flavor).network_guarantee,
                    'network_limit', (v_flavor).network_limit,
                    'io_limit', code.dynamic_io_limit((v_host).space_limit, v_disk_type, (v_flavor).io_limit),
                    'io_cores_limit', (v_flavor).io_cores_limit,
                    'name', (v_flavor).name,
                    'description', (v_flavor).name,
                    'vtype', (v_flavor).vtype,
                    'type', (v_flavor).type,
                    'generation', (v_flavor).generation,
                    'platform_id', (v_flavor).platform_id
                ),
                'space_limit', (v_host).space_limit,
                'disk_type_id', (v_disk_type).disk_type_ext_id,
                'geo', (v_geo).name,
                'region', (v_region).name,
                'cloud_provider', (v_region).cloud_provider,
                'assign_public_ip', (v_host).assign_public_ip,
                'created_at', (v_host).created_at,
                'on_dedicated_host', coalesce(cardinality((v_cluster).host_group_ids), 0) > 0
            ),
            'versions', jsonb_build_object()
        ),
        'yandex', jsonb_build_object('environment', (v_cluster).env));

    IF (v_cluster).type::text = 'greenplum_cluster' THEN
        v_derived_config = jsonb_set(
                v_derived_config, '{data,dbaas,cluster_hosts_info}', v_cluster_hosts);
    END IF;

    FOREACH component IN ARRAY (v_subcluster).roles
    LOOP
        v_derived_config = jsonb_insert(v_derived_config, '{data,runlist,-1}',
                                        ('"components.' || component || '"')::jsonb, true);
    END LOOP;

    FOR v_version IN
        SELECT * FROM code.get_rev_version_by_host(i_fqdn, v_rev)
    LOOP
        v_derived_config = jsonb_set(
            v_derived_config, array_append('{data,versions}', (v_version).component),
            jsonb_build_object(
                'major_version', (v_version).major_version,
                'minor_version', (v_version).minor_version,
                'package_version', (v_version).package_version,
                'edition', (v_version).edition));
    END LOOP;

    FOR host IN
        SELECT
            sc.subcid,
            sc.name AS subcluster_name,
            sc.roles,
            s.name AS shard_name,
            s.shard_id,
            h.fqdn,
            g.name "geo"
        FROM
            dbaas.subclusters_revs sc
            JOIN dbaas.hosts_revs h ON (h.subcid = sc.subcid AND h.rev = sc.rev)
            LEFT JOIN dbaas.shards_revs s ON (s.shard_id = h.shard_id AND s.rev = h.rev)
            JOIN dbaas.geo g ON (h.geo_id = g.geo_id)
        WHERE
            sc.rev = v_rev
            AND sc.cid = (v_cluster).cid
        ORDER BY h.fqdn
    LOOP
        v_derived_config = jsonb_insert(v_derived_config, '{data,dbaas,cluster_hosts,-1}',
                                        ('"' || (host).fqdn || '"')::jsonb, true);

        IF ((v_host).shard_id IS NULL AND (host).shard_id IS NULL) OR (v_host).shard_id = (host).shard_id THEN
            v_derived_config = jsonb_insert(v_derived_config, '{data,dbaas,shard_hosts,-1}',
                                            ('"' || (host).fqdn || '"')::jsonb, true);
        END IF;

        IF v_derived_config->'data'->'dbaas'->'cluster'->'subclusters'->(host).subcid IS NULL THEN
            v_derived_config = jsonb_set(
                v_derived_config, array_append('{data,dbaas,cluster,subclusters}', (host).subcid),
                jsonb_build_object(
                    'name', (host).subcluster_name,
                    'roles', array_to_json((host).roles),
                    'shards', jsonb_build_object(),
                    'hosts', jsonb_build_object()));
        END IF;

        IF (host).shard_id IS NOT NULL THEN
            IF v_derived_config->'data'->'dbaas'->'cluster'->'subclusters'->(host).subcid->'shards'->(host).shard_id IS NULL THEN
                v_derived_config = jsonb_set(
                    v_derived_config,
                    array_append(
                        array_append(
                            array_append('{data,dbaas,cluster,subclusters}', (host).subcid),
                            'shards'),
                        (host).shard_id),
                    jsonb_build_object(
                        'name', (host).shard_name,
                        'hosts', jsonb_build_object()));
            END IF;

            v_derived_config = jsonb_set(
                v_derived_config,
                array_append(
                    array_append(
                        array_append(
                            array_append(
                                array_append('{data,dbaas,cluster,subclusters}', (host).subcid),
                                'shards'),
                            (host).shard_id),
                        'hosts'),
                    (host).fqdn),
                jsonb_build_object(
                    'geo', (host).geo));
        ELSE
            v_derived_config = jsonb_set(
                v_derived_config,
                array_append(
                    array_append(
                        array_append('{data,dbaas,cluster,subclusters}', (host).subcid),
                        'hosts'),
                    (host).fqdn),
                jsonb_build_object(
                    'geo', (host).geo));
        END IF;
    END LOOP;

    RETURN code.combine_dict(array_append(array_append(v_config, v_custom_pillar), v_derived_config));
END;
$$ LANGUAGE plpgsql STABLE;

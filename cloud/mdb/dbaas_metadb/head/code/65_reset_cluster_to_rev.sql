CREATE OR REPLACE FUNCTION code.reset_cluster_to_rev(
    i_cid    text,
    i_rev    bigint
) RETURNS void AS $$
DECLARE
    v_head_coords code.cluster_coords;
    v_rev_coords  code.cluster_coords;

    v_old_cluster_revs dbaas.clusters_revs;
BEGIN
    SELECT *
    INTO v_old_cluster_revs
    FROM dbaas.clusters_revs
    WHERE cid = i_cid
      AND rev = i_rev;

    IF NOT found THEN
        RAISE EXCEPTION 'Unable to find clusters_revs cid=%, rev=%', i_cid, i_rev
            USING TABLE = 'dbaas.cluster_revs';
    END IF;

    v_head_coords := code.get_coords(i_cid);
    v_rev_coords  := code.get_coords_at_rev(i_cid, i_rev);

    -- delete all from pillar to subcluster
    DELETE FROM dbaas.cluster_labels
    WHERE cid = i_cid;

    DELETE FROM dbaas.pillar
    WHERE fqdn IS NOT NULL
      AND fqdn = ANY((v_head_coords).fqdns);

    DELETE FROM dbaas.pillar
    WHERE shard_id IS NOT NULL
      AND shard_id = ANY((v_head_coords).shard_ids);

    DELETE FROM dbaas.pillar
    WHERE subcid IS NOT NULL
      AND subcid = ANY((v_head_coords).subcids);

    DELETE FROM dbaas.pillar
    WHERE cid = i_cid;

    DELETE FROM dbaas.disks
    WHERE cid = i_cid;

    DELETE FROM dbaas.hosts
    WHERE fqdn = ANY((v_head_coords).fqdns);

    DELETE FROM dbaas.shards
    WHERE shard_id = ANY((v_head_coords).shard_ids);

    DELETE FROM dbaas.instance_groups
    WHERE subcid = ANY((v_head_coords).subcids);

    DELETE FROM dbaas.kubernetes_node_groups
    WHERE subcid = ANY((v_head_coords).subcids);

    DELETE FROM dbaas.sgroups
    WHERE cid = i_cid;

    DELETE FROM dbaas.subclusters
    WHERE subcid = ANY((v_head_coords).subcids);

    DELETE FROM dbaas.maintenance_window_settings
    WHERE cid = i_cid;

    DELETE FROM dbaas.backup_schedule
    WHERE cid = i_cid;

    DELETE FROM dbaas.versions
    WHERE cid IS NOT NULL
      AND cid = i_cid;

    DELETE FROM dbaas.versions
    WHERE shard_id IS NOT NULL
      AND shard_id = ANY((v_head_coords).shard_ids);

    DELETE FROM dbaas.versions
    WHERE subcid IS NOT NULL
      AND subcid = ANY((v_head_coords).subcids);

    DELETE FROM dbaas.disk_placement_groups
    WHERE cid = i_cid;

    DELETE FROM dbaas.placement_groups
    WHERE cid = i_cid;

    -- restore from subcluster to pillar
    INSERT INTO dbaas.subclusters
    (cid, subcid, name, roles, created_at)
    SELECT
        cid, subcid, name, roles, created_at
    FROM dbaas.subclusters_revs
    WHERE subcid = ANY((v_rev_coords).subcids)
      AND rev = i_rev;

    INSERT INTO dbaas.instance_groups
    (instance_group_id, subcid)
    SELECT
        instance_group_id, subcid
    FROM dbaas.instance_groups_revs
    WHERE subcid = ANY((v_rev_coords).subcids)
      AND rev = i_rev;

    INSERT INTO dbaas.kubernetes_node_groups
    (kubernetes_cluster_id, node_group_id, subcid)
    SELECT
        kubernetes_cluster_id, node_group_id, subcid
    FROM dbaas.kubernetes_node_groups_revs
    WHERE subcid = ANY((v_rev_coords).subcids)
      AND rev = i_rev;

    INSERT INTO dbaas.sgroups
    (cid, sg_ext_id, sg_type, sg_hash, sg_allow_all)
    SELECT
        cid, sg_ext_id, sg_type, sg_hash, sg_allow_all
    FROM dbaas.sgroups_revs
    WHERE cid = i_cid
      AND rev = i_rev;

    INSERT INTO dbaas.shards
    (subcid, shard_id, name, created_at)
    SELECT
        subcid, shard_id, name, created_at
    FROM dbaas.shards_revs
    WHERE shard_id = ANY((v_rev_coords).shard_ids)
      AND rev = i_rev;

    INSERT INTO dbaas.hosts
    (subcid, shard_id, flavor, space_limit, fqdn, vtype_id,
     geo_id, disk_type_id, subnet_id, assign_public_ip, created_at,
     is_in_user_project_id)
    SELECT
        subcid, shard_id, flavor, space_limit, fqdn, vtype_id,
        geo_id, disk_type_id, subnet_id, assign_public_ip, created_at,
        is_in_user_project_id
    FROM dbaas.hosts_revs
    WHERE fqdn = ANY((v_rev_coords).fqdns)
      AND rev = i_rev;

    INSERT INTO dbaas.pillar
    (cid, value)
    SELECT cid, value
    FROM dbaas.pillar_revs
    WHERE cid IS NOT NULL
      AND cid = i_cid
      AND rev = i_rev;

    INSERT INTO dbaas.pillar
    (subcid, value)
    SELECT subcid, value
    FROM dbaas.pillar_revs
    WHERE subcid IS NOT NULL
      AND subcid = ANY((v_rev_coords).subcids)
      AND rev = i_rev;

    INSERT INTO dbaas.pillar
    (shard_id, value)
    SELECT shard_id, value
    FROM dbaas.pillar_revs
    WHERE shard_id IS NOT NULL
      AND shard_id = ANY((v_rev_coords).shard_ids)
      AND rev = i_rev;

    INSERT INTO dbaas.pillar
    (fqdn, value)
    SELECT fqdn, value
    FROM dbaas.pillar_revs
    WHERE fqdn IS NOT NULL
      AND fqdn = ANY((v_rev_coords).fqdns)
      AND rev = i_rev;

    INSERT INTO dbaas.cluster_labels
    (cid, label_key, label_value)
    SELECT
        cid, label_key, label_value
    FROM dbaas.cluster_labels_revs
    WHERE cid = i_cid
      AND rev = i_rev;

    INSERT INTO dbaas.maintenance_window_settings
    (cid, day, hour)
    SELECT
        cid, day, hour
    FROM dbaas.maintenance_window_settings_revs
    WHERE cid = i_cid
      AND rev = i_rev;

    INSERT INTO dbaas.backup_schedule
    (cid, schedule)
    SELECT
        cid, schedule
    FROM dbaas.backup_schedule_revs
    WHERE cid = i_cid
      AND rev = i_rev;

    INSERT INTO dbaas.versions
    ( cid, subcid, shard_id, component, major_version, minor_version, package_version, edition, pinned )
    SELECT
        cid, subcid, shard_id, component, major_version, minor_version, package_version, edition, pinned
    FROM dbaas.versions_revs
    WHERE cid = i_cid
      AND rev = i_rev;

    INSERT INTO dbaas.versions
    ( cid, subcid, shard_id, component, major_version, minor_version, package_version, edition, pinned )
    SELECT
        cid, subcid, shard_id, component, major_version, minor_version, package_version, edition, pinned
    FROM dbaas.versions_revs
    WHERE subcid IS NOT NULL
      AND subcid = ANY((v_rev_coords).subcids)
      AND rev = i_rev;

    INSERT INTO dbaas.versions
    ( cid, subcid, shard_id, component, major_version, minor_version, package_version, edition, pinned )
    SELECT
        cid, subcid, shard_id, component, major_version, minor_version, package_version, edition, pinned
    FROM dbaas.versions_revs
    WHERE shard_id IS NOT NULL
      AND shard_id = ANY((v_rev_coords).shard_ids)
      AND rev = i_rev;

    INSERT INTO dbaas.disk_placement_groups
    ( pg_id, cid, local_id, disk_placement_group_id, status )
    SELECT
        pg_id, cid, local_id, disk_placement_group_id, status
    FROM dbaas.disk_placement_groups_revs
    WHERE cid = i_cid
      AND rev = i_rev;

    INSERT INTO dbaas.disks
    ( d_id, pg_id, fqdn, mount_point, disk_id, host_disk_id, status, cid )
    SELECT
        d_id, pg_id, fqdn, mount_point, disk_id, host_disk_id, status, cid
    FROM dbaas.disks_revs
    WHERE cid = i_cid
      AND rev = i_rev;

    INSERT INTO dbaas.placement_groups
    ( pg_id, cid, subcid, shard_id, placement_group_id, status )
    SELECT
        pg_id, cid, subcid, shard_id, placement_group_id, status
    FROM dbaas.placement_groups_revs
    WHERE cid = i_cid
      AND rev = i_rev;

    UPDATE dbaas.clusters
    SET name = (v_old_cluster_revs).name,
        description = (v_old_cluster_revs).description,
        network_id = (v_old_cluster_revs).network_id,
        folder_id = (v_old_cluster_revs).folder_id,
        status = (v_old_cluster_revs).status,
        host_group_ids = (v_old_cluster_revs).host_group_ids,
        monitoring_cloud_id = (v_old_cluster_revs).monitoring_cloud_id
    WHERE cid = i_cid;
END;
$$ LANGUAGE plpgsql;

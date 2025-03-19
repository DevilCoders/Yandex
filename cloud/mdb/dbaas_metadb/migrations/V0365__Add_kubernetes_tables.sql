CREATE TABLE dbaas.kubernetes_node_groups
(
    subcid                   text NOT NULL,
    kubernetes_cluster_id    text,
    node_group_id text,

    CONSTRAINT pk_kubernetes_node_groups PRIMARY KEY (subcid),
    CONSTRAINT fk_kubernetes_node_groups_subclusters FOREIGN KEY (subcid)
        REFERENCES dbaas.subclusters
);
CREATE INDEX i_kubernetes_node_groups_kubernetes_cluster_id ON dbaas.kubernetes_node_groups (kubernetes_cluster_id);
CREATE INDEX i_kubernetes_node_groups_node_group_id ON dbaas.kubernetes_node_groups (node_group_id);

CREATE TABLE dbaas.kubernetes_node_groups_revs
(
    subcid                text   NOT NULL,
    kubernetes_cluster_id text,
    node_group_id         text,
    rev                   bigint NOT NULL,

    CONSTRAINT pk_kubernetes_node_groups_revs PRIMARY KEY (subcid, rev),
    CONSTRAINT fk_kubernetes_node_groups_revs_subclusters_rev FOREIGN KEY (subcid, rev)
        REFERENCES dbaas.subclusters_revs
);

GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.kubernetes_node_groups TO backup_cli;
GRANT SELECT ON TABLE dbaas.kubernetes_node_groups TO cloud_dwh;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.kubernetes_node_groups TO cms;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.kubernetes_node_groups TO dbaas_api;
GRANT SELECT ON TABLE dbaas.kubernetes_node_groups TO dbaas_support;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.kubernetes_node_groups TO dbaas_worker;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.kubernetes_node_groups TO idm_service;
GRANT SELECT ON TABLE dbaas.kubernetes_node_groups TO katan_imp;
GRANT SELECT ON TABLE dbaas.kubernetes_node_groups TO logs_api;
GRANT SELECT ON TABLE dbaas.kubernetes_node_groups TO mdb_health;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.kubernetes_node_groups TO mdb_maintenance;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.kubernetes_node_groups TO mdb_ui;
GRANT SELECT ON TABLE dbaas.kubernetes_node_groups TO pillar_config;
GRANT SELECT ON TABLE dbaas.kubernetes_node_groups TO pillar_secrets;

GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.kubernetes_node_groups_revs TO backup_cli;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.kubernetes_node_groups_revs TO cms;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.kubernetes_node_groups_revs TO dbaas_api;
GRANT SELECT ON TABLE dbaas.kubernetes_node_groups_revs TO dbaas_support;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.kubernetes_node_groups_revs TO dbaas_worker;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.kubernetes_node_groups_revs TO idm_service;
GRANT SELECT ON TABLE dbaas.kubernetes_node_groups_revs TO katan_imp;
GRANT SELECT ON TABLE dbaas.kubernetes_node_groups_revs TO logs_api;
GRANT SELECT ON TABLE dbaas.kubernetes_node_groups_revs TO mdb_health;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.kubernetes_node_groups_revs TO mdb_maintenance;
GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE dbaas.kubernetes_node_groups_revs TO mdb_ui;
GRANT SELECT ON TABLE dbaas.kubernetes_node_groups_revs TO pillar_config;
GRANT SELECT ON TABLE dbaas.kubernetes_node_groups_revs TO pillar_secrets;

-- 57_add_kubernetes_subcluster.sql
CREATE OR REPLACE FUNCTION code.add_kubernetes_subcluster(
    i_cid         text,
    i_subcid      text,
    i_name        text,
    i_roles       dbaas.role_type[],
    i_rev         bigint
) RETURNS TABLE (cid text, subcid text, name text, roles dbaas.role_type[]) AS $$
BEGIN
    PERFORM code.add_subcluster(
        i_cid,
        i_subcid,
        i_name,
        i_roles,
        i_rev
    );

    INSERT INTO dbaas.kubernetes_node_groups (
        subcid
    ) VALUES (
        i_subcid
    );

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'add_kubernetes_subcluster',
            jsonb_build_object(
                'subcid', i_subcid,
                'name', i_name,
                'roles', i_roles::text[]
            )
        )
    );

    RETURN QUERY SELECT i_cid, i_subcid, i_name, i_roles;
END;
$$ LANGUAGE plpgsql;

GRANT ALL ON FUNCTION code.add_kubernetes_subcluster(i_cid text, i_subcid text, i_name text, i_roles dbaas.role_type[], i_rev bigint) TO backup_cli;
GRANT ALL ON FUNCTION code.add_kubernetes_subcluster(i_cid text, i_subcid text, i_name text, i_roles dbaas.role_type[], i_rev bigint) TO cms;
GRANT ALL ON FUNCTION code.add_kubernetes_subcluster(i_cid text, i_subcid text, i_name text, i_roles dbaas.role_type[], i_rev bigint) TO dbaas_api;
GRANT ALL ON FUNCTION code.add_kubernetes_subcluster(i_cid text, i_subcid text, i_name text, i_roles dbaas.role_type[], i_rev bigint) TO dbaas_support;
GRANT ALL ON FUNCTION code.add_kubernetes_subcluster(i_cid text, i_subcid text, i_name text, i_roles dbaas.role_type[], i_rev bigint) TO dbaas_worker;
GRANT ALL ON FUNCTION code.add_kubernetes_subcluster(i_cid text, i_subcid text, i_name text, i_roles dbaas.role_type[], i_rev bigint) TO idm_service;
GRANT ALL ON FUNCTION code.add_kubernetes_subcluster(i_cid text, i_subcid text, i_name text, i_roles dbaas.role_type[], i_rev bigint) TO katan_imp;
GRANT ALL ON FUNCTION code.add_kubernetes_subcluster(i_cid text, i_subcid text, i_name text, i_roles dbaas.role_type[], i_rev bigint) TO logs_api;
GRANT ALL ON FUNCTION code.add_kubernetes_subcluster(i_cid text, i_subcid text, i_name text, i_roles dbaas.role_type[], i_rev bigint) TO mdb_downtimer;
GRANT ALL ON FUNCTION code.add_kubernetes_subcluster(i_cid text, i_subcid text, i_name text, i_roles dbaas.role_type[], i_rev bigint) TO mdb_health;
GRANT ALL ON FUNCTION code.add_kubernetes_subcluster(i_cid text, i_subcid text, i_name text, i_roles dbaas.role_type[], i_rev bigint) TO mdb_maintenance;
GRANT ALL ON FUNCTION code.add_kubernetes_subcluster(i_cid text, i_subcid text, i_name text, i_roles dbaas.role_type[], i_rev bigint) TO mdb_ui;
GRANT ALL ON FUNCTION code.add_kubernetes_subcluster(i_cid text, i_subcid text, i_name text, i_roles dbaas.role_type[], i_rev bigint) TO pillar_config;
GRANT ALL ON FUNCTION code.add_kubernetes_subcluster(i_cid text, i_subcid text, i_name text, i_roles dbaas.role_type[], i_rev bigint) TO pillar_secrets;

-- 57_update_kubernetes_node_group.sql
CREATE
OR REPLACE FUNCTION code.update_kubernetes_node_group(
    i_cid                   text,
    i_kubernetes_cluster_id text,
    i_node_group_id         text,
    i_subcid                text,
    i_rev                   bigint
) RETURNS void AS $$
BEGIN
    UPDATE dbaas.kubernetes_node_groups
    SET kubernetes_cluster_id = i_kubernetes_cluster_id,
        node_group_id = i_node_group_id
    WHERE subcid = i_subcid;

    IF
    NOT found THEN
            RAISE EXCEPTION 'Unable to find subcluster %', i_subcid
                  USING TABLE = 'dbaas.kubernetes_node_groups';
    END IF;

        PERFORM code.update_cluster_change(
            i_cid,
            i_rev,
            jsonb_build_object(
                'update_kubernetes_node_group',
                jsonb_build_object(
                    'subcid', i_subcid,
                    'kubernetes_cluster_id', i_kubernetes_cluster_id,
                    'node_group_id', i_node_group_id
                )
            )
        );

END;
$$
LANGUAGE plpgsql;

GRANT ALL ON FUNCTION code.update_kubernetes_node_group(i_cid text, i_kubernetes_cluster_id text, i_node_group_id text, i_subcid text, i_rev bigint) TO backup_cli;
GRANT ALL ON FUNCTION code.update_kubernetes_node_group(i_cid text, i_kubernetes_cluster_id text, i_node_group_id text, i_subcid text, i_rev bigint) TO cms;
GRANT ALL ON FUNCTION code.update_kubernetes_node_group(i_cid text, i_kubernetes_cluster_id text, i_node_group_id text, i_subcid text, i_rev bigint) TO dbaas_api;
GRANT ALL ON FUNCTION code.update_kubernetes_node_group(i_cid text, i_kubernetes_cluster_id text, i_node_group_id text, i_subcid text, i_rev bigint) TO dbaas_support;
GRANT ALL ON FUNCTION code.update_kubernetes_node_group(i_cid text, i_kubernetes_cluster_id text, i_node_group_id text, i_subcid text, i_rev bigint) TO dbaas_worker;
GRANT ALL ON FUNCTION code.update_kubernetes_node_group(i_cid text, i_kubernetes_cluster_id text, i_node_group_id text, i_subcid text, i_rev bigint) TO idm_service;
GRANT ALL ON FUNCTION code.update_kubernetes_node_group(i_cid text, i_kubernetes_cluster_id text, i_node_group_id text, i_subcid text, i_rev bigint) TO katan_imp;
GRANT ALL ON FUNCTION code.update_kubernetes_node_group(i_cid text, i_kubernetes_cluster_id text, i_node_group_id text, i_subcid text, i_rev bigint) TO logs_api;
GRANT ALL ON FUNCTION code.update_kubernetes_node_group(i_cid text, i_kubernetes_cluster_id text, i_node_group_id text, i_subcid text, i_rev bigint) TO mdb_downtimer;
GRANT ALL ON FUNCTION code.update_kubernetes_node_group(i_cid text, i_kubernetes_cluster_id text, i_node_group_id text, i_subcid text, i_rev bigint) TO mdb_health;
GRANT ALL ON FUNCTION code.update_kubernetes_node_group(i_cid text, i_kubernetes_cluster_id text, i_node_group_id text, i_subcid text, i_rev bigint) TO mdb_maintenance;
GRANT ALL ON FUNCTION code.update_kubernetes_node_group(i_cid text, i_kubernetes_cluster_id text, i_node_group_id text, i_subcid text, i_rev bigint) TO mdb_ui;
GRANT ALL ON FUNCTION code.update_kubernetes_node_group(i_cid text, i_kubernetes_cluster_id text, i_node_group_id text, i_subcid text, i_rev bigint) TO pillar_config;
GRANT ALL ON FUNCTION code.update_kubernetes_node_group(i_cid text, i_kubernetes_cluster_id text, i_node_group_id text, i_subcid text, i_rev bigint) TO pillar_secrets;

-- 20_to_rev.sql
CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.kubernetes_node_groups,
    bigint
) RETURNS dbaas.kubernetes_node_groups_revs AS $$
SELECT
    $1.subcid,
    $1.kubernetes_cluster_id,
    $1.node_group_id,
    $2;
$$ LANGUAGE SQL IMMUTABLE;

GRANT ALL ON FUNCTION code.to_rev(dbaas.kubernetes_node_groups, bigint) TO backup_cli;
GRANT ALL ON FUNCTION code.to_rev(dbaas.kubernetes_node_groups, bigint) TO cms;
GRANT ALL ON FUNCTION code.to_rev(dbaas.kubernetes_node_groups, bigint) TO dbaas_api;
GRANT ALL ON FUNCTION code.to_rev(dbaas.kubernetes_node_groups, bigint) TO dbaas_support;
GRANT ALL ON FUNCTION code.to_rev(dbaas.kubernetes_node_groups, bigint) TO dbaas_worker;
GRANT ALL ON FUNCTION code.to_rev(dbaas.kubernetes_node_groups, bigint) TO idm_service;
GRANT ALL ON FUNCTION code.to_rev(dbaas.kubernetes_node_groups, bigint) TO katan_imp;
GRANT ALL ON FUNCTION code.to_rev(dbaas.kubernetes_node_groups, bigint) TO logs_api;
GRANT ALL ON FUNCTION code.to_rev(dbaas.kubernetes_node_groups, bigint) TO mdb_downtimer;
GRANT ALL ON FUNCTION code.to_rev(dbaas.kubernetes_node_groups, bigint) TO mdb_health;
GRANT ALL ON FUNCTION code.to_rev(dbaas.kubernetes_node_groups, bigint) TO mdb_maintenance;
GRANT ALL ON FUNCTION code.to_rev(dbaas.kubernetes_node_groups, bigint) TO mdb_ui;
GRANT ALL ON FUNCTION code.to_rev(dbaas.kubernetes_node_groups, bigint) TO pillar_config;
GRANT ALL ON FUNCTION code.to_rev(dbaas.kubernetes_node_groups, bigint) TO pillar_secrets;

-- 65_reset_cluster_to_rev.sql
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
     geo_id, disk_type_id, subnet_id, assign_public_ip, created_at)
    SELECT
        subcid, shard_id, flavor, space_limit, fqdn, vtype_id,
        geo_id, disk_type_id, subnet_id, assign_public_ip, created_at
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

-- 56_delete_subcluster.sql
CREATE OR REPLACE FUNCTION code.delete_subcluster(
    i_cid         text,
    i_subcid      text,
    i_rev         bigint
) RETURNS void AS $$
BEGIN
    DELETE FROM dbaas.pillar
     WHERE subcid = i_subcid;
    DELETE FROM dbaas.instance_groups
     WHERE subcid = i_subcid;
    DELETE FROM dbaas.kubernetes_node_groups
     WHERE subcid = i_subcid;
    DELETE FROM dbaas.subclusters
     WHERE subcid = i_subcid;

    PERFORM code.update_cluster_change(
        i_cid,
        i_rev,
        jsonb_build_object(
            'delete_subcluster',
             jsonb_build_object(
                'subcid', i_subcid
            )
        )
    );
END;
$$ LANGUAGE plpgsql;

-- 55_complete_cluster_change.sql
CREATE OR REPLACE FUNCTION code.complete_cluster_change(
    i_cid  text,
    i_rev  bigint
) RETURNS void AS $$
DECLARE
    v_coords code.cluster_coords;
BEGIN
    IF NOT EXISTS (
        SELECT 1
          FROM dbaas.clusters_changes
         WHERE cid = i_cid
           AND rev = i_rev) THEN
        RAISE EXCEPTION 'Unable to find cluster change cid=%, rev=%', i_cid, i_rev
            USING TABLE = 'dbaas.cluster_changes',
                   HINT = 'initiate cluster change with lock_cluster';
    END IF;

    v_coords := code.get_coords(i_cid);

    INSERT INTO dbaas.clusters_revs
    SELECT fmt.*
      FROM dbaas.clusters c,
           code.to_rev(c) fmt
     WHERE c.cid = i_cid;

    INSERT INTO dbaas.subclusters_revs
    SELECT fmt.*
      FROM dbaas.subclusters s,
           code.to_rev(s, i_rev) fmt
     WHERE s.subcid = ANY((v_coords).subcids);

    INSERT INTO dbaas.shards_revs
    SELECT fmt.*
      FROM dbaas.shards s,
           code.to_rev(s, i_rev) fmt
     WHERE s.shard_id = ANY((v_coords).shard_ids);

    INSERT INTO dbaas.hosts_revs
    SELECT fmt.*
      FROM dbaas.hosts h,
           code.to_rev(h, i_rev) fmt
     WHERE h.fqdn = ANY((v_coords).fqdns);

    INSERT INTO dbaas.pillar_revs
    SELECT fmt.*
      FROM dbaas.pillar p,
           code.to_rev(p, i_rev) fmt
     WHERE p.cid IS NOT NULL
       AND p.cid = i_cid;

    INSERT INTO dbaas.pillar_revs
    SELECT fmt.*
      FROM dbaas.pillar p,
           code.to_rev(p, i_rev) fmt
     WHERE p.subcid IS NOT NULL
       AND p.subcid = ANY((v_coords).subcids);

    INSERT INTO dbaas.pillar_revs
    SELECT fmt.*
      FROM dbaas.pillar p,
           code.to_rev(p, i_rev) fmt
     WHERE p.shard_id IS NOT NULL
       AND p.shard_id = ANY((v_coords).shard_ids);

    INSERT INTO dbaas.pillar_revs
    SELECT fmt.*
      FROM dbaas.pillar p,
           code.to_rev(p, i_rev) fmt
     WHERE p.fqdn IS NOT NULL
       AND p.fqdn = ANY((v_coords).fqdns);

    INSERT INTO dbaas.cluster_labels_revs
    SELECT fmt.*
      FROM dbaas.cluster_labels l,
           code.to_rev(l, i_rev) fmt
     WHERE l.cid = i_cid;

    INSERT INTO dbaas.maintenance_window_settings_revs
    SELECT fmt.*
    FROM dbaas.maintenance_window_settings s,
         code.to_rev(s, i_rev) fmt
    WHERE s.cid = i_cid;

    INSERT INTO dbaas.backup_schedule_revs
    SELECT fmt.*
      FROM dbaas.backup_schedule b,
           code.to_rev(b, i_rev) fmt
     WHERE b.cid = i_cid;

    INSERT INTO dbaas.instance_groups_revs
    SELECT fmt.*
      FROM dbaas.instance_groups i,
           code.to_rev(i, i_rev) fmt
     WHERE i.subcid = ANY((v_coords).subcids);

    INSERT INTO dbaas.kubernetes_node_groups_revs
    SELECT fmt.*
      FROM dbaas.kubernetes_node_groups i,
           code.to_rev(i, i_rev) fmt
     WHERE i.subcid = ANY((v_coords).subcids);

    INSERT INTO dbaas.sgroups_revs
    SELECT fmt.*
      FROM dbaas.sgroups i,
           code.to_rev(i, i_rev) fmt
     WHERE i.cid = i_cid;

    INSERT INTO dbaas.versions_revs
    SELECT fmt.*
      FROM dbaas.versions v,
           code.to_rev(v, i_rev) fmt
     WHERE v.cid IS NOT NULL
       AND v.cid = i_cid;

    INSERT INTO dbaas.versions_revs
    SELECT fmt.*
      FROM dbaas.versions v,
           code.to_rev(v, i_rev) fmt
     WHERE v.subcid IS NOT NULL
       AND v.subcid = ANY((v_coords).subcids);

    INSERT INTO dbaas.versions_revs
    SELECT fmt.*
      FROM dbaas.versions v,
           code.to_rev(v, i_rev) fmt
     WHERE v.shard_id IS NOT NULL
       AND v.shard_id = ANY((v_coords).shard_ids);

    INSERT INTO dbaas.disk_placement_groups_revs
    SELECT fmt.*
      FROM dbaas.disk_placement_groups v,
           code.to_rev(v, i_rev) fmt
     WHERE v.cid = i_cid;

    INSERT INTO dbaas.disks_revs
    SELECT fmt.*
      FROM dbaas.disks v,
           code.to_rev(v, i_rev) fmt
     WHERE v.cid = i_cid;

	INSERT INTO dbaas.alert_group_revs
    SELECT fmt.*
      FROM dbaas.alert_group v,
           code.to_rev(v, i_rev) fmt
     WHERE v.cid = i_cid;

    INSERT INTO dbaas.placement_groups_revs
    SELECT fmt.*
      FROM dbaas.placement_groups v,
           code.to_rev(v, i_rev) fmt
     WHERE v.cid = i_cid;
END;
$$ LANGUAGE plpgsql;

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

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.clusters
) RETURNS dbaas.clusters_revs AS $$
SELECT
    $1.cid,
    $1.next_rev,
    $1.name,
    $1.network_id,
    $1.folder_id,
    $1.description,
    $1.status,
    $1.host_group_ids,
    $1.monitoring_cloud_id;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.subclusters,
    bigint
) RETURNS dbaas.subclusters_revs AS $$
SELECT
    $1.subcid,
    $2,
    $1.cid,
    $1.name,
    $1.roles,
    $1.created_at;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.shards,
    bigint
) RETURNS dbaas.shards_revs AS $$
SELECT
    $1.subcid,
    $1.shard_id,
    $2,
    $1.name,
    $1.created_at;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.hosts,
    bigint
) RETURNS dbaas.hosts_revs AS $$
SELECT
    $1.subcid,
    $1.shard_id,
    $1.flavor,
    $1.space_limit,
    $1.fqdn,
    $2,
    $1.vtype_id,
    $1.geo_id,
    $1.disk_type_id,
    $1.subnet_id,
    $1.assign_public_ip,
    $1.created_at,
    $1.is_in_user_project_id;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.pillar,
    bigint
) RETURNS dbaas.pillar_revs AS $$
SELECT
    $2,
    $1.cid,
    $1.subcid,
    $1.shard_id,
    $1.fqdn,
    $1.value;
$$ LANGUAGE SQL IMMUTABLE;


CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.cluster_labels,
    bigint
) RETURNS dbaas.cluster_labels_revs AS $$
SELECT
    $1.cid,
    $1.label_key,
    $2,
    $1.label_value;
$$ LANGUAGE SQL IMMUTABLE;


CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.maintenance_window_settings,
    bigint
) RETURNS dbaas.maintenance_window_settings_revs AS $$
SELECT
    $1.cid,
    $2,
    $1.day,
    $1.hour;
$$ LANGUAGE SQL IMMUTABLE;


CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.backup_schedule,
    bigint
) RETURNS dbaas.backup_schedule_revs AS $$
SELECT
    $1.cid,
    $2,
    $1.schedule;
$$ LANGUAGE SQL IMMUTABLE;


CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.instance_groups,
    bigint
) RETURNS dbaas.instance_groups_revs AS $$
SELECT
    $1.subcid,
    $1.instance_group_id,
    $2;
$$ LANGUAGE SQL IMMUTABLE;


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


CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.sgroups,
    bigint
) RETURNS dbaas.sgroups_revs AS $$
SELECT
    $1.cid,
    $1.sg_ext_id,
    $1.sg_type,
    $2,
    $1.sg_hash,
    $1.sg_allow_all;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.versions,
    bigint
) RETURNS dbaas.versions_revs AS $$
SELECT
    $1.cid,
    $1.subcid,
    $1.shard_id,
    $1.component,
    $1.major_version,
    $1.minor_version,
    $1.package_version,
    $2,
    $1.edition,
    $1.pinned;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.disk_placement_groups,
    bigint
) RETURNS dbaas.disk_placement_groups_revs AS $$
SELECT
    $2,
    $1.pg_id,
    $1.cid,
    $1.local_id,
    $1.disk_placement_group_id,
    $1.status;
$$ LANGUAGE SQL IMMUTABLE;


CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.disks,
    bigint
) RETURNS dbaas.disks_revs AS $$
SELECT
    $2,
    $1.d_id,
    $1.pg_id,
    $1.fqdn,
    $1.mount_point,
    $1.disk_id,
    $1.host_disk_id,
    $1.status,
    $1.cid;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.alert_group,
    bigint
) RETURNS dbaas.alert_group_revs AS $$
SELECT
    $1.alert_group_id,
    $2,
    $1.cid,
    $1.monitoring_folder_id,
    $1.managed,
    $1.status;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.alert,
    bigint
) RETURNS dbaas.alert_revs AS $$
SELECT
    $1.alert_ext_id,
    $1.alert_group_id,
    $1.template_id,
    $2,
    $1.notification_channels,
    $1.disabled,
    $1.critical_threshold,
    $1.warning_threshold,
    $1.default_thresholds,
    $1.status;
$$ LANGUAGE SQL IMMUTABLE;

CREATE OR REPLACE FUNCTION code.to_rev(
    dbaas.placement_groups,
    bigint
) RETURNS dbaas.placement_groups_revs AS $$
SELECT
    $1.pg_id,
    $2,
    $1.cid,
    $1.subcid,
    $1.shard_id,
    $1.placement_group_id,
    $1.status,
    $1.fqdn,
    $1.local_id
$$ LANGUAGE SQL IMMUTABLE;


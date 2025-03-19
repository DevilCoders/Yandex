$src_table = {{ param["source_table_path"]->quote() }};
$dst_table = {{ input1->table_quote() }};

--всего справочника
--$start_at = CAST(Date('2017-01-01') as DateTime);
$end_at = CAST(Date('2099-12-31') as DateTime);
--текущая загрузка
$end_current_at = CAST(CurrentUtcTimestamp() as DateTime);
$start_current_at = $end_current_at+ DateTime::IntervalFromSeconds(1);

$default_quotas_new = (
    SELECT
    String::ReplaceAll(t.quota_name,'_','-')    AS quota_name
    , t.quota_limit                             AS quota_limit
    , t.unit                                    AS unit
    , $start_current_at                         AS start_at
    , $end_at                                   AS end_at
    FROM (
    VALUES
        ('instances', 12,''),
        ('instance_disks', 8,''),
        ('instance_disks_per_numa_node', 8,''),
        ('instance_k8s_disks', 64,''),
        ('instance_network_interfaces', 1,''),
        ('instance_network_interfaces_per_numa_node', 8,''),
        ('instance_cores', 3200,''),
        ('gpus', 0,''),
        ('cores', 100000,''),
        ('memory', 128,'G'),
        ('placement_groups', 2,''),
        ('spread_placement_group_instances', 5,''),
        ('disk_placement_groups', 5,''),
        ('templates', 100,''),
        ('snapshots', 32,''),
        ('total_snapshot_size', 400,'G'),
        ('nbs_disks', 32,''),
        ('total_disk_size', 200,'G'),
        ('network_hdd_total_disk_size', 500,'G'),
        ('network_ssd_total_disk_size', 200,'G'),
        ('network_ssd_nonreplicated_total_disk_size', 558,'G'),
        ('filesystems', 100,''),
        ('network_ssd_total_filesystem_size', 0,''),
        ('network_hdd_total_filesystem_size', 0,''),
        ('images', 32,''),
        ('networks', 2,''),
        ('subnets', 12,''),
        ('target_groups', 100,''),
        ('network_load_balancers', 2,''),
        ('external_addresses', 8,''),
        ('external_qrator_addresses', 100000,''),
        ('external_smtp_direct_addresses', 0,''),
        ('external_static_addresses', 2,''),
        ('route_tables', 8,''),
        ('security_groups', 10,''),
        ('static_routes', 256,''),
        ('host_groups', 0,''),
        ('host_group_hosts', 3,''),
        ('disk_pools', 0,''),
        ('network_ssd_total_removed_disk_size', 2199023255552,''),
        ('network_ssd_nonreplicated_total_removed_disk_size', 2199023255552,''),
        ('network_hdd_total_removed_disk_size', 10995116277760,''),
        ('network_ssd_pct_removed_disk_size', 10,''),
        ('network_ssd_nonreplicated_pct_removed_disk_size', 10,''),
        ('network_hdd_pct_removed_disk_size', 5,'')
    ) AS t(quota_name,quota_limit,unit)
);

$default_quotas_old = (
SELECT *
    FROM $dst_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
-- Исторические данные end_at < $end_at
SELECT
    quota_name                                              AS quota_name,
    quota_limit                                             AS quota_limit,
    unit                                                    AS unit,
    start_at                                                AS start_at,
    end_at                                                  AS end_at
FROM
    $default_quotas_old
    WHERE end_at < $end_at
-- Исторические данные end_at = $end_at
UNION ALL
SELECT
    old.quota_name                                          AS quota_name,
    old.quota_limit                                         AS quota_limit,
    old.unit                                                AS unit,
    old.start_at                                            AS start_at,
    IF(new.quota_name IS Null,$end_current_at,old.end_at)   AS end_at
FROM
    $default_quotas_old AS old
LEFT JOIN $default_quotas_new AS new
        ON old.quota_name = new.quota_name AND
        old.quota_limit=new.quota_limit AND
        old.unit=new.unit
    WHERE old.end_at = $end_at
--Новые данные
UNION ALL
SELECT
    new.quota_name                                          AS quota_name,
    new.quota_limit                                         AS quota_limit,
    new.unit                                                AS unit,
    new.start_at                                            AS start_at,
    new.end_at                                              AS end_at
FROM
    $default_quotas_new AS new
LEFT JOIN $default_quotas_old AS old
    on new.quota_name = old.quota_name AND
        new.quota_limit=old.quota_limit AND
        new.unit=old.unit AND
        new.end_at = old.end_at
WHERE old.quota_name IS NULL
;

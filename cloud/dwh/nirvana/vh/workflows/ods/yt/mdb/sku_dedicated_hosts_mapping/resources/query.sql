$src_table = {{ param["source_table_path"]->quote() }};
$dst_table = {{ input1->table_quote() }};

--всего справочника
--$start_dttm = CAST(Date('2017-01-01') as DateTime); 
$end_dttm = CAST(Date('2099-12-31') as DateTime);
--текущая загрузка
$end_current_dttm = CAST(CurrentUtcTimestamp() as DateTime); 
$start_current_dttm = $end_current_dttm+ DateTime::IntervalFromSeconds(1);

$default_quotas_new = (
    SELECT 
        t.mdb_sku_dedic_compute_name                        AS mdb_sku_dedic_compute_name,
        t.mdb_sku_dedic_mdb_name                            AS mdb_sku_dedic_mdb_name,
        $start_current_dttm                                 AS start_dttm,
        $end_dttm                                           AS end_dttm
    FROM (
    VALUES
        ('compute.hostgroup.cpu.c100.v1', 'mdb.cluster.greenplum.v2.cpu.c100.dedicated'),
        ('compute.hostgroup.cpu.c100.v3', 'mdb.cluster.greenplum.v3.cpu.c100.dedicated'),
        ('compute.hostgroup.ram.v1'     , 'mdb.cluster.greenplum.v2.ram.dedicated'     ),
        ('compute.hostgroup.ram.v3'     , 'mdb.cluster.greenplum.v3.ram.dedicated'     ),
        ('compute.hostgroup.localssd.v1', 'mdb.cluster.local-nvme.greenplum.dedicated' )
    ) AS t(mdb_sku_dedic_compute_name,mdb_sku_dedic_mdb_name)
);

$default_quotas_old = (
SELECT *
    FROM $src_table
);

/* Save result in ODS */
INSERT INTO $dst_table WITH TRUNCATE
-- Исторические данные end_dttm < $end_dttm
SELECT 
    mdb_sku_dedic_compute_name                              AS mdb_sku_dedic_compute_name,
    mdb_sku_dedic_mdb_name                                  AS mdb_sku_dedic_mdb_name,
    start_dttm                                              AS start_dttm,
    end_dttm                                                AS end_dttm
FROM 
    $default_quotas_old
    WHERE end_dttm < $end_dttm
-- Исторические данные end_dttm = $end_dttm
UNION ALL
SELECT 
    old.mdb_sku_dedic_compute_name                              AS mdb_sku_dedic_compute_name,
    old.mdb_sku_dedic_mdb_name                                  AS mdb_sku_dedic_mdb_name,
    old.start_dttm                                              AS start_dttm,
    IF(new.mdb_sku_dedic_compute_name IS Null,$end_current_dttm,old.end_dttm)       AS end_dttm
FROM 
    $default_quotas_old AS old
LEFT JOIN $default_quotas_new AS new 
    ON  old.mdb_sku_dedic_compute_name = new.mdb_sku_dedic_compute_name AND 
        old.mdb_sku_dedic_mdb_name=new.mdb_sku_dedic_mdb_name
    WHERE old.end_dttm = $end_dttm
--Новые данные
UNION ALL
SELECT 
    new.mdb_sku_dedic_compute_name                          AS mdb_sku_dedic_compute_name,
    new.mdb_sku_dedic_mdb_name                              AS mdb_sku_dedic_mdb_name,
    new.start_dttm                                          AS start_dttm,
    new.end_dttm                                            AS end_dttm
FROM 
    $default_quotas_new AS new
LEFT JOIN $default_quotas_old AS old 
    ON  old.mdb_sku_dedic_compute_name = new.mdb_sku_dedic_compute_name AND 
        old.mdb_sku_dedic_mdb_name=new.mdb_sku_dedic_mdb_name AND
        new.end_dttm = old.end_dttm
WHERE old.end_dttm IS NULL
;

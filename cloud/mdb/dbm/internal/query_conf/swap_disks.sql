WITH placeholder_volumes AS (
    SELECT
        path,
        disk_id,
        dom0
    FROM
        mdb.volumes
    WHERE
        container = %(destination)s
        AND disk_id IS NOT NULL)
UPDATE
    mdb.volumes v
SET
    disk_id = pv.disk_id,
    dom0_path = '/disks/' || pv.disk_id::text || '/' || %(source)s,
    dom0 = pv.dom0
FROM
    placeholder_volumes pv
WHERE
    v.path = pv.path
    AND v.container = %(source)s

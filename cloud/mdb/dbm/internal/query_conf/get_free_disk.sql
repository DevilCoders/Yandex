SELECT
    id
FROM
    mdb.disks
WHERE
    dom0 = %(dom0)s
    AND has_data = false
ORDER BY id
LIMIT 1
FOR NO KEY UPDATE
SKIP LOCKED

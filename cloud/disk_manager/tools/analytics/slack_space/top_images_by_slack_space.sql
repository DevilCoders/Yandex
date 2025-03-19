SELECT iid, bsize, osize, (1.0 * bsize - osize) AS diff
FROM
(
    SELECT b.iid AS iid, MAX(b.total_size) / 1024 / 1024 / 1024 AS bsize, MAX(o.total_size) / 1024 / 1024 / 1024 AS osize
    FROM
    (
        SELECT
            image_id AS iid, SUM(IF(size IS NULL OR size = 0, CAST(4 AS Uint64) * 1024 * 1024 * 1024 * 1024, size)) AS total_size
        FROM `pools/base_disks`
        WHERE status = 2
        GROUP BY image_id
    ) AS b
    JOIN
    (
        SELECT image_id AS iid, SUM(overlay_disk_size) AS total_size
        FROM `pools/slots`
        WHERE status = 0
        GROUP BY image_id
    ) AS o
    ON b.iid = o.iid
    GROUP BY b.iid
)
ORDER BY diff DESC
LIMIT 10

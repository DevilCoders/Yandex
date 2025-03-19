INSERT INTO mdb.volume_backups (
    container,
    path,
    dom0,
    dom0_path,
    space_limit,
    disk_id
) SELECT
    container,
    path,
    dom0,
    dom0_path,
    space_limit,
    disk_id
FROM mdb.volumes
WHERE
    container = %(container)s
    AND path IN %(paths)s
ON CONFLICT ON CONSTRAINT pk_volume_backups
DO UPDATE SET create_ts = now()
RETURNING path

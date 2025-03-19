data:
    walg:
        ssh_server: pg-backup05f.db.yandex.net
    pgsync:
        zk_hosts: zkeeper04f.db.yandex.net:2181,zkeeper04h.db.yandex.net:2181,zkeeper04k.db.yandex.net:2181
        remaster_restart: 'no'

include:
    - mdb_s3_porto_prod.kcache

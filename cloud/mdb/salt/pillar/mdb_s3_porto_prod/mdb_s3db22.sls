data:
    walg:
        ssh_server: pg-backup02f.db.yandex.net
    pgsync:
        zk_hosts: zkeeper05f.db.yandex.net:2181,zkeeper05h.db.yandex.net:2181,zkeeper05k.db.yandex.net:2181
        remaster_restart: 'no'

include:
    - mdb_s3_porto_prod.kcache

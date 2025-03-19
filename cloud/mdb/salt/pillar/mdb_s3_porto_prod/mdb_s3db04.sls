data:
    walg:
        ssh_server: pg-backup07f.db.yandex.net
    pgsync:
        zk_hosts: zkeeper03f.db.yandex.net:2181,zkeeper03h.db.yandex.net:2181,zkeeper03k.db.yandex.net:2181
        remaster_restart: 'no'

include:
    - mdb_s3_porto_prod.kcache

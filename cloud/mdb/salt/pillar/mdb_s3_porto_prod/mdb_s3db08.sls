data:
    walg:
        ssh_server: pg-backup01f.db.yandex.net
    pgsync:
        zk_hosts: zkeeper02e.db.yandex.net:2181,zkeeper02h.db.yandex.net:2181,zkeeper02k.db.yandex.net:2181
        remaster_restart: 'no'

include:
    - mdb_s3_porto_prod.kcache

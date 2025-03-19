include:
    - envs.prod
    - porto.prod.common_zookeeper:
          defaults:
              zkeeper_net: _C_MDB_ZKEEPER02_
              zkeeper_ctype: zookeeper02

data:
    zk:
        jvm_xmx: '16G'
        nodes:
            zkeeper02e.db.yandex.net: 1
            zkeeper02h.db.yandex.net: 2
            zkeeper02k.db.yandex.net: 3

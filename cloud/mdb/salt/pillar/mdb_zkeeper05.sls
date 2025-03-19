include:
    - envs.prod
    - porto.prod.common_zookeeper:
          defaults:
              zkeeper_net: _C_MDB_ZKEEPER05_
              zkeeper_ctype: zookeeper05

data:
    zk:
        jvm_xmx: '16G'
        nodes:
            zkeeper05h.db.yandex.net: 1
            zkeeper05k.db.yandex.net: 2
            zkeeper05f.db.yandex.net: 3

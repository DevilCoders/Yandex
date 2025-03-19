include:
    - envs.prod
    - porto.prod.common_zookeeper:
          defaults:
              zkeeper_net: _C_MDB_ZKEEPER04_
              zkeeper_ctype: zookeeper04

data:
    zk:
        jvm_xmx: '16G'
        nodes:
            zkeeper04h.db.yandex.net: 1
            zkeeper04k.db.yandex.net: 2
            zkeeper04f.db.yandex.net: 3

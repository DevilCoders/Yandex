include:
    - envs.prod
    - porto.prod.common_zookeeper:
          defaults:
              zkeeper_net: _C_MDB_ZKEEPER_
              zkeeper_ctype: zookeeper01

data:
    zk:
        jvm_xmx: '16G'
        nodes:
            zkeeper01e.db.yandex.net: 1
            zkeeper01f.db.yandex.net: 2
            zkeeper01h.db.yandex.net: 3

include:
    - envs.prod
    - porto.prod.common_zookeeper:
          defaults:
              zkeeper_net: _C_MDB_ZKEEPER03_
              zkeeper_ctype: zookeeper03

data:
    zk:
        jvm_xmx: '16G'
        nodes:
            zkeeper03h.db.yandex.net: 1
            zkeeper03k.db.yandex.net: 2
            zkeeper03f.db.yandex.net: 3

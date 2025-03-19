include:
    - envs.dev
    - porto.prod.common_zookeeper:
          defaults:
              zkeeper_net: _C_MDB_ZKEEPER_TEST02_
              zkeeper_ctype: zookeeper-test02

data:
    zk:
        jvm_xmx: '3G'
        nodes:
            zkeeper-test02e.db.yandex.net: 1
            zkeeper-test02h.db.yandex.net: 2
            zkeeper-test02k.db.yandex.net: 3

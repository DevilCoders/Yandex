include:
    - envs.dev
    - porto.prod.common_zookeeper:
          defaults:
              zkeeper_net: _C_MDB_ZKEEPER_TEST_
              zkeeper_ctype: zookeeper-test01

data:
    zk:
        jvm_xmx: '3G'
        nodes:
            zkeeper-test01e.db.yandex.net: 1
            zkeeper-test01f.db.yandex.net: 2
            zkeeper-test01h.db.yandex.net: 3

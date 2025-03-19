include:
    - envs.qa
    - porto.prod.common_zookeeper:
          defaults:
              zkeeper_net: _C_MDB_ZK_DF_E2E_
              zkeeper_ctype: zk_df_e2e

data:
    zk:
        jvm_xmx: '512M'
        nodes:
            zk-df-e2e01h.db.yandex.net: 1
            zk-df-e2e01f.db.yandex.net: 2
            zk-df-e2e01k.db.yandex.net: 3

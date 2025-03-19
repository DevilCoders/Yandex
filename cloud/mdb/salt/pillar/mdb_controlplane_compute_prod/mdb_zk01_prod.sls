include:
    - mdb_controlplane_compute_prod.mdb_zk_prod

data:
    zk:
        nodes:
            zk-dbaas01f.yandexcloud.net: 1
            zk-dbaas01h.yandexcloud.net: 2
            zk-dbaas01k.yandexcloud.net: 3

firewall:
    ACCEPT:
      - net: zk-dbaas01f.yandexcloud.net
        type: fqdn
        ports:
          - '2888'
          - '3888'
      - net: zk-dbaas01h.yandexcloud.net
        type: fqdn
        ports:
          - '2888'
          - '3888'
      - net: zk-dbaas01k.yandexcloud.net
        type: fqdn
        ports:
          - '2888'
          - '3888'


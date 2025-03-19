include:
    - mdb_controlplane_compute_prod.mdb_zk_prod

data:
    zk:
        nodes:
            zk-dbaas02f.yandexcloud.net: 1
            zk-dbaas02h.yandexcloud.net: 2
            zk-dbaas02k.yandexcloud.net: 3

firewall:
    ACCEPT:
      - net: zk-dbaas02f.yandexcloud.net
        type: fqdn
        ports:
          - '2888'
          - '3888'
      - net: zk-dbaas02h.yandexcloud.net
        type: fqdn
        ports:
          - '2888'
          - '3888'
      - net: zk-dbaas02k.yandexcloud.net
        type: fqdn
        ports:
          - '2888'
          - '3888'

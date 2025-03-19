include:
  - porto.prod.selfdns.realm-mdb
  - porto.prod.dbaas.solomon

data:
    runlist:
        - components.zk
        - components.firewall
    zk:
        version: '3.5.5-1+yandex19-3067ff6'
        nodes: {}
        config:
            dataDir: /data
            snapCount: 1000000
            fsync.warningthresholdms: 500
            maxSessionTimeout: 60000
            autopurge.purgeInterval: 1  # Purge hourly
            reconfigEnabled: 'false'
    monrun2: True
    mdb_metrics:
        main:
            yasm_tags_cmd: '/usr/local/yasmagent/mdb_zk_getter.py'
            yasm_tags_db: zookeeper
    solomon:
        agent: https://solomon.yandex-team.ru/push/json
        push_url: https://solomon.yandex-team.ru/api/v2/push
        project: internal-mdb
        cluster: 'mdb_{ctype}'
        service: mdb
    billing:
        ship_logs: False
    dbaas:
        vtype: porto
        cluster_id: {{ zkeeper_ctype }}
        cluster_name: {{ zkeeper_ctype }}
        cluster_type: zookeeper_cluster
        fqdn: {{ salt.grains.get('fqdn') }}

firewall:
    policy: ACCEPT
    ACCEPT:
      - net: {{ zkeeper_net }}
        type: macro
        ports:
          - '2888'
          - '3888'
    REJECT:
      - net: '::/0'
        type: addr6
        ports:
          # Quorum port
          - '2888'
          # Cluster intercommunication port.
          - '3888'
      - net: '0/0'
        type: addr4
        ports:
          - '2888'
          - '3888'

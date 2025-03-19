include:
    - envs.prod
    - porto.prod.selfdns.realm-mail
    - porto.prod.dbaas.solomon

data:
    condgroup: mail_zk_mops
    l3host: True
    monrun2: True
    runlist:
        - components.zk
    zk:
{% if salt['grains.get']('os') == 'RedHat' %}
        version: '3.4.6-1'
{% else %}
        version: '3.5.5-1+yandex19-3067ff6'
{% endif %}
        jvm_xmx: '16G'
        nodes:
            'zk01e.mail.yandex.net': 1
            'zk01f.mail.yandex.net': 2
            'zk01h.mail.yandex.net': 3
        config:
            dataDir: /data
            snapCount: 1000000
            autopurge.snapRetainCount: 20  # Snapshot count
            autopurge.purgeInterval: 1  # Purge hourly
            reconfigEnabled: 'false'
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
    pushclient:
        start_service: false
    billing:
        ship_logs: False
    dbaas:
        vtype: porto
        cluster_id: mail_zk01
        cluster_name: mail_zk01
        cluster_type: zookeeper_cluster
        fqdn: {{ salt.grains.get('fqdn') }}
firewall:
    policy: ACCEPT
    ACCEPT:
      - net: zk01f.mail.yandex.net
        type: fqdn
        ports:
          - '2888'
          - '3888'
      - net: zk01h.mail.yandex.net
        type: fqdn
        ports:
          - '2888'
          - '3888'
      - net: zk01e.mail.yandex.net
        type: fqdn
        ports:
          - '2888'
          - '3888'
      - net: _C_MAIL_ZK_MOPS_
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

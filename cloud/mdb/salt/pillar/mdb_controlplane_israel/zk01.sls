{% from "mdb_controlplane_israel/map.jinja" import clusters with context %}
{% set zk_nodes = {} %}
{% for f in clusters.zk01.fqdns | sort %}
{%    do zk_nodes.update({f: loop.index}) %}
{% endfor %}

include:
    - envs.compute-prod
    - mdb_controlplane_israel.common

data:
    runlist:
        - components.zk
        - components.linux-kernel
        - components.deploy.agent
    dbaas_compute:
        vdb_setup: False
    zk:
        version: '3.5.5-1+yandex19-3067ff6'
        jvm_xmx: {{ clusters.zk01.resources.memory // 2 }}G
        nodes: {{ zk_nodes | tojson }}
        config:
            snapCount: 1000000
            fsync.warningthresholdms: 500
            maxSessionTimeout: 60000
            autopurge.purgeInterval: 1  # Purge hourly
            reconfigEnabled: 'false'

firewall:
    policy: ACCEPT
    ACCEPT:
{% for f in clusters.zk01.fqdns | sort %}
      - net: {{ f }}
        type: fqdn
        ports:
          # Quorum port
          - '2888'
          # Cluster intercommunication port.
          - '3888'
{% endfor %}
    REJECT:
      - net: ::/0
        type: addr6
        ports:
          # Quorum port
          - '2888'
          # Cluster intercommunication port.
          - '3888'
      - net: 0/0
        type: addr4
        ports:
          - '2888'
          - '3888'


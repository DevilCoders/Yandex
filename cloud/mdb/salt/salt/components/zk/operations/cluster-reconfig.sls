{% if salt.pillar.get('zookeeper-join', None) != None %}
zookeeper.cleanup.zookeeper-join:
    cmd.run:
        - name: /usr/local/yandex/zk_cluster_reconfig.py zookeeper-join --zid {{ salt.pillar.get('zookeeper-join:zid', '') }} --fqdn {{ salt.pillar.get('zookeeper-join:fqdn', []) }}
        - env:
          - LC_ALL: C.UTF-8
          - LANG: C.UTF-8
{% endif %}

{% if salt.pillar.get('zookeeper-leave', None) != None %}
zookeeper.cleanup.zookeeper-leave:
    cmd.run:
        - name: /usr/local/yandex/zk_cluster_reconfig.py zookeeper-leave --zid {{ salt.pillar.get('zookeeper-leave:zid', '') }}
        - env:
          - LC_ALL: C.UTF-8
          - LANG: C.UTF-8
{% endif %}

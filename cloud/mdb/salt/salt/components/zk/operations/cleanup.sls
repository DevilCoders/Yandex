{% if salt.pillar.get('clickhouse-hosts', None) != None %}
zookeeper.cleanup.clickhouse-hosts:
    cmd.run:
        - name: /usr/local/yandex/zk_cleanup.py clickhouse-hosts --root {{ '/clickhouse/' + salt.pillar.get('clickhouse-hosts:cid', '') }} --fqdn {{ ','.join(salt.pillar.get('clickhouse-hosts:removed-hosts', [])) }}
        - env:
          - LC_ALL: C.UTF-8
          - LANG: C.UTF-8
{% endif %}

{% if salt.pillar.get('zookeeper-nodes', None) != None %}
zookeeper.cleanup.zookeeper-nodes:
    cmd.run:
        - name: /usr/local/yandex/zk_cleanup.py zookeeper-nodes --path {{ ','.join(salt.pillar.get('zookeeper-nodes:removed-nodes', [])) }}
        - env:
          - LC_ALL: C.UTF-8
          - LANG: C.UTF-8
{% endif %}

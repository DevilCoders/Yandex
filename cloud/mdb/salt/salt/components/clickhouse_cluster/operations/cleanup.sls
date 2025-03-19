{% if salt.pillar.get('clickhouse-hosts', None) != None %}
zookeeper.cleanup.clickhouse-hosts:
    cmd.run:
    - name: /usr/local/yandex/zk_cleanup.py clickhouse-hosts --root {{ salt.mdb_clickhouse.zookeeper_root() }} --fqdn {{ ','.join(salt.pillar.get('clickhouse-hosts:removed-hosts', [])) }}
    - env:
        - LC_ALL: C.UTF-8
        - LANG: C.UTF-8
{% endif %}

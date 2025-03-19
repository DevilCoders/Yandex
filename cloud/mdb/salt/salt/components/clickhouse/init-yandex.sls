include:
    - .yasmagent
    - components.monrun2.clickhouse
{% if salt.pillar.get('data:mdb_metrics:enabled', True) %}
    - .mdb-metrics
{% else %}
    - components.mdb-metrics.disable
{% endif %}
    - components.pushclient2
    - .pushclient
{% if salt.pillar.get('data:dbaas:vtype') == 'compute' and salt.pillar.get('data:dbaas:cluster') %}
    - components.firewall
    - components.firewall.external_access_config
    - .firewall
{% endif %}

extend:
    clickhouse-pushclient-req:
        test.nop:
            - require:
                - pkg: pushclient
                - user: statbox-user

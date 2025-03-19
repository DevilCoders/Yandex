{% if salt.dbaas.is_aws() %}
/etc/ferm/conf.d/20_allow_support_access.conf:
    file.managed:
        - source: salt://components/firewall/conf/conf.d/20_datacloud_allow_support_access.conf
        - template: jinja
        - mode: 644
        - context:
            process_user: clickhouse
        - require:
            - test: ferm-ready
        - watch_in:
            - cmd: reload-ferm-rules
/etc/ferm/conf.d/30_restict_external_access.conf:
    file.managed:
        - source: salt://{{ slspath }}/datacloud_firewall_rules.iptables
        - template: jinja
        - mode: 644
        - require:
            - test: ferm-ready
        - watch_in:
            - cmd: reload-ferm-rules
{% else %}
/etc/ferm/conf.d/10_restict_external_comm.conf:
    file.managed:
        - source: salt://{{ slspath }}/clickhouse-server.iptables
        - template: jinja
        - mode: 644
        - require:
            - test: ferm-ready
        - watch_in:
            - cmd: reload-ferm-rules

/etc/ferm/conf.d/30_user_net_routes.conf:
    file.managed:
        - source: salt://{{ slspath }}/user-net-routes.iptables
        - template: jinja
        - mode: 644
        - require:
            - test: ferm-ready
        - watch_in:
            - cmd: reload-ferm-rules
{% endif %}

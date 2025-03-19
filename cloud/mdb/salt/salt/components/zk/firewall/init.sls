{% if salt.dbaas.is_aws() %}
ipv6-resolved:
    dns.wait_for_ipv6:
        - fqdn: {{ salt.grains.get('id') }}
        - wait_timeout: 600

/etc/ferm/conf.d/20_allow_support_access.conf:
    file.managed:
        - source: salt://components/firewall/conf/conf.d/20_datacloud_allow_support_access.conf
        - template: jinja
        - mode: 644
        - context:
            process_user: zookeeper
        - require:
            - test: ferm-ready
            - ipv6-resolved
        - watch_in:
            - cmd: reload-ferm-rules
/etc/ferm/conf.d/30_restict_external_access.conf:
    file.managed:
        - source: salt://{{ slspath }}/datacloud_firewall_rules.iptables
        - template: jinja
        - mode: 644
        - require:
            - test: ferm-ready
            - ipv6-resolved
        - watch_in:
            - cmd: reload-ferm-rules
{% else %}
/etc/ferm/conf.d/20_worker_access.conf:
    file.managed:
        - source: salt://{{ slspath }}/zk-worker.iptables
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
            - user: zookeeper-user
        - watch_in:
            - cmd: reload-ferm-rules
{% endif %}

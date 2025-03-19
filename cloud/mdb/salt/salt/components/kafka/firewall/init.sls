{% if salt.dbaas.is_aws() %}

ipv4-resolved:
    dns.wait_for_ipv4:
        - fqdn: {{ salt.grains.get('id') }}
        - wait_timeout: 600

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
            process_user: kafka
        - require:
            - test: ferm-ready
            - ipv4-resolved
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
            - ipv4-resolved
            - ipv6-resolved
        - watch_in:
            - cmd: reload-ferm-rules
{% endif %}

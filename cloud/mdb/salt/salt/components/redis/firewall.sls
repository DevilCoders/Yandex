/etc/ferm/conf.d/10_restict_external_comm.conf:
    file.managed:
        - source: salt://{{ slspath }}/common/conf/redis.iptables
        - template: jinja
        - mode: 644
        - require:
            - test: ferm-ready
        - watch_in:
            - cmd: reload-ferm-rules

/etc/ferm/conf.d/30_block_outgoing.conf:
    file.managed:
        - source: salt://{{ slspath }}/common/conf/block-outgoing.iptables
        - template: jinja
        - mode: 644
        - require:
            - test: ferm-ready
        - watch_in:
            - cmd: reload-ferm-rules

/etc/ferm/conf.d/30_user_net_routes.conf:
    file.managed:
        - source: salt://{{ slspath }}/common/conf/user-net-routes.iptables
        - template: jinja
        - mode: 644
        - require:
            - test: ferm-ready
        - watch_in:
            - cmd: reload-ferm-rules

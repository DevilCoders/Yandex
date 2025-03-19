/etc/ferm/conf.d/10_restict_external_comm.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/postgresql.iptables
        - template: jinja
        - mode: 644
        - require:
            - test: ferm-ready
        - watch_in:
            - cmd: reload-ferm-rules

/etc/ferm/conf.d/30_user_net_routes.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/user-net-routes.iptables
        - template: jinja
        - mode: 644
        - require:
            - test: ferm-ready
        - watch_in:
            - cmd: reload-ferm-rules

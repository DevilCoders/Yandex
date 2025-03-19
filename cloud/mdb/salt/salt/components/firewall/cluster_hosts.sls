/etc/ferm/conf.d/15_cluster_hosts.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/conf.d/15_cluster_hosts.conf
        - template: jinja
        - mode: 644
        - require:
            - test: ferm-ready
        - watch_in:
            - cmd: reload-ferm-rules

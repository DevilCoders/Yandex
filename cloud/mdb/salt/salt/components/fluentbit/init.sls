fluentbit-packages:
    pkg.installed:
        - refresh: False
        - pkgs:
            - td-agent-bit: 1.8.8

/opt/td-agent-bit/bin/yc-logging.so:
    file.managed:
        - source: https://storage.yandexcloud.net/dataproc/fluentbit-yc-plugin/yc-logging.1.1.0.so
        - source_hash: 60cde2bf584eace6a733c0417b7e2399d1259e2df01cc63fc537d0fa58eb072d
        - require:
            - pkg: fluentbit-packages

/etc/td-agent-bit/plugins.conf:
    file.managed:
        - makedirs: true
        - show_changes: false
        - source: salt://{{ slspath }}/conf/plugins.conf
        - require:
            - file: /opt/td-agent-bit/bin/yc-logging.so

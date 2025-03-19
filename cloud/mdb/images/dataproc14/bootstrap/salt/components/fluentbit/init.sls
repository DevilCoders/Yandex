{%- set disable_cloud_logging = salt['pillar.get']('data:properties:dataproc:disable_cloud_logging', False) -%}

{% if salt['ydputils.is_presetup']() %}
fluentbit-repository:
    pkgrepo.managed:
        - name: deb https://packages.fluentbit.io/ubuntu/xenial xenial main
        - file: /etc/apt/sources.list.d/fluentbit.list
        - key_url: https://packages.fluentbit.io/fluentbit.key
        - gpgcheck: 1
        - refresh: True

fluentbit-packages:
    pkg.installed:
        - refresh: False
        - pkgs:
            - td-agent-bit: 1.8.8
        - require:
            - pkgrepo: fluentbit-repository

/etc/init.d/td-agent-bit:
    file.managed:
        - makedirs: true
        - show_changes: false
        - mode: 755
        - source: salt://{{ slspath }}/conf/init.d/td-agent-bit
        - require:
            - pkg: fluentbit-packages

/etc/logrotate.d/td-agent-bit:
    file.managed:
        - makedirs: true
        - show_changes: false
        - mode: 755
        - source: salt://{{ slspath }}/conf/logrotate.d/td-agent-bit
        - require:
            - pkg: fluentbit-packages

/lib/systemd/system/td-agent-bit.service:
    file.absent

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

service-fluentbit-disabled:
    service.disabled:
        - name: td-agent-bit
        - require:
            - pkg: fluentbit-packages

{% elif salt['pillar.get']('data:logging:url') and not disable_cloud_logging -%}

/etc/td-agent-bit/parsers.conf:
    file.managed:
        - makedirs: true
        - show_changes: false
        - source: salt://{{ slspath }}/conf/parsers.conf

/etc/td-agent-bit/td-agent-bit.conf:
    file.managed:
        - template: jinja
        - makedirs: true
        - show_changes: false
        - source: salt://{{ slspath }}/conf/td-agent-bit.conf

service-fluentbit:
    service.running:
        - enable: true
        - name: td-agent-bit
        - require:
            - file: /etc/td-agent-bit/td-agent-bit.conf
            - file: /etc/td-agent-bit/parsers.conf
            - file: /usr/local/share/ca-certificates/yandex-cloud-ca.crt
        - watch:
            - file: /etc/td-agent-bit/td-agent-bit.conf
            - file: /etc/td-agent-bit/parsers.conf

{% else %}

service-fluentbit-disabled:
    service.disabled:
        - name: td-agent-bit

service-fluentbit-stopped:
    service.dead:
        - name: td-agent-bit

{% endif %}

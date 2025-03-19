{% set config_path = salt['pillar.get']('data:config:zeppelin_config_path', '/etc/zeppelin/conf') %}

{% if salt['ydputils.is_masternode']() %}
service-zeppelin-server:
    service:
        - running
        - enable: true
        - name: zeppelin
        - parallel: true
        - require:
            - pkg: zeppelin_packages
            - file: {{ config_path }}/zeppelin-env.sh
            - file: {{ config_path }}/interpreter.json
            - hadoop-property: {{ config_path }}/zeppelin-site.xml
        - watch:
            - pkg: zeppelin_packages
            - file: {{ config_path }}/zeppelin-env.sh
            - file: {{ config_path }}/interpreter.json
            - hadoop-property: {{ config_path }}/zeppelin-site.xml
{% endif %}

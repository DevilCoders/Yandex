{% set config_path = salt['pillar.get']('data:config:knox_config_path', '/etc/knox/conf') %}

{% if salt['ydputils.is_masternode']() %}
service-knox-server:
    service:
        - running
        - enable: true
        - name: knox
        - watch:
            - pkg: knox_packages
            - file: {{ config_path }}/topologies/default-topology.xml
        - require:
            - cmd: knox-generate-master-secret
            - pkg: knox_packages
            - file: {{ config_path }}/topologies/default-topology.xml
{% endif %}

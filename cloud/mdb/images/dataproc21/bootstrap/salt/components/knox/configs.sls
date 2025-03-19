{% set config_path = salt['pillar.get']('data:config:knox_config_path', '/etc/knox/conf') %}
{% set masternode = salt['ydputils.get_masternodes']()[0] %}

{% if salt['ydputils.is_masternode']() -%}

{{ config_path }}/topologies/default-topology.xml:
    file.managed:
        - name: {{ config_path }}/topologies/default-topology.xml
        - source: salt://{{ slspath }}/conf/default-topology.xml
        - template: jinja
        - user: knox
        - group: knox
        - require:
            - pkg: knox_packages
        - defaults:
            masternode: '{{ masternode }}'
{%- endif %}

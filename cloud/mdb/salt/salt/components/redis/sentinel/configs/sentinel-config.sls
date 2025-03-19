{% from "components/redis/sentinel/map.jinja" import sentinel with context %}
{% set master_name = salt.pillar.get('data:dbaas:cluster_name', 'mymaster') %}
{% set restart = salt.pillar.get('service-restart') or salt.pillar.get('sentinel-restart')%}

sentinel-user:
  user.present:
    - name: {{ sentinel.user }}
    - gid: {{ sentinel.group }}
    - system: True

sentinel-config:
    file.managed:
        - name: /etc/redis/sentinel.conf
        - template: jinja
        - source: salt://components/redis/sentinel/conf/sentinel.conf
        - user: {{ sentinel.user }}
        - group: {{ sentinel.group }}
        - defaults:
            config: {{ sentinel.config|json }}
            master_config: {{ sentinel.master_config|json }}
            master_name: {{ master_name }}
        - require:
            - user: sentinel-user
{% if not restart and not salt.pillar.get('redis-master') %}
        - unless:
            - fgrep -q {{ master_name }} /etc/redis/sentinel.conf
{% endif %}

{% from "components/redis/server/map.jinja" import redis with context %}

redis-group:
  group.present:
    - name: {{ redis.group }}
    - system: True

redis-user:
  user.present:
    - name: {{ redis.user }}
    - gid: {{ redis.group }}
    - system: True
    - require:
      - group: redis-group

# We fill initial configs before installing packages
# to avoid split-brain on install phase
redis-config-init:
    file.managed:
        - name: {{ salt.mdb_redis.get_config_name() }}
        - user: {{ redis.user }}
        - group: {{ redis.group }}
        - mode: 640
        - makedirs: True
        - dir_mode: 750
        - template: jinja
        - source: salt://components/redis/server/conf/redis.conf
{% if not salt.pillar.get('redis-master') %}
        - unless:
            - grep 'include /etc/redis/redis-main.conf' {{ salt.mdb_redis.get_config_name() }}
{% endif %}
        - require:
            - user: redis-user

include:
    - .redis-main-config
    - ...common.salt_pkgs

extend:
    redis-main-config:
        file.managed:
            - user: {{ redis.user }}
            - group: {{ redis.group }}
            - mode: 640
            - require:
                - pkg: redis-salt-module-requirements
                - file: redis-config-init

redis-config-dir:
    file.directory:
        - name: {{ salt.mdb_redis.get_redis_data_folder() }}
        - user: {{ redis.user }}
        - group: {{ redis.group }}
        - mode: 750
        - file_mode: 644
        - recurse:
            - user
            - group
            - mode

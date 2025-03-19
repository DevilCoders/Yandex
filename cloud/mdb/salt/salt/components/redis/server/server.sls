{% from "components/redis/server/map.jinja" import redis with context %}
{% set aof_enabled = salt.mdb_redis.is_aof_enabled() %}

remove-old-init-file:
    file.absent:
        - name: /etc/init.d/redis-server

remove-default-systemd-unit-file:
    file.absent:
        - name: /etc/systemd/system/redis-server.service

redis-server-init:
    file.managed:
        - name: /lib/systemd/system/redis-server.service
        - template: jinja
        - source: salt://{{ slspath }}/conf/redis-server.service
        - mode: 644
        - require:
            - file: /usr/local/yandex/start-pre.sh
            - file: /usr/local/yandex/start-redis.sh
            - file: /usr/local/yandex/redisctl
{% if salt.dbaas.is_dataplane() %}
            - file: etc-fix-init
{% endif %}
{% if salt.mdb_redis.is_managed_cluster() %}
            - file: /usr/local/yandex/ensure_no_primary.sh
            - file: /usr/local/yandex/start-post.sh
            - file: /usr/local/yandex/stop.sh
{% endif %}
        - onchanges_in:
            - module: systemd-reload

/usr/local/yandex/start-redis.sh:
    file.managed:
        - template: jinja
        - require:
            - pkg: redis-pkgs
            - file: redis-main-config
            - file: /etc/default/redis-server
        - source: salt://{{ slspath }}/conf/start-redis.sh
        - mode: 755
        - onchanges_in:
            - module: systemd-reload

{% if salt.mdb_redis.is_managed_cluster() %}
/usr/local/yandex/start-post.sh:
    file.managed:
        - template: jinja
        - require:
            - pkg: redis-pkgs
            - file: redis-main-config
            - file: /etc/default/redis-server
            - file: /usr/local/yandex/redisctl
        - source: salt://{{ slspath }}/conf/start-post.sh
        - mode: 755
        - onchanges_in:
            - module: systemd-reload

/usr/local/yandex/stop.sh:
    file.managed:
        - template: jinja
        - require:
            - pkg: redis-pkgs
            - file: redis-main-config
            - file: /etc/default/redis-server
            - file: /usr/local/yandex/redisctl
            - file: /usr/local/yandex/ensure_no_primary.sh
        - source: salt://{{ slspath }}/conf/stop.sh
        - mode: 755
        - onchanges_in:
            - module: systemd-reload

{% endif %}

/etc/systemd/system/redis-server.service:
    file.absent

/etc/default/redis-server:
    file.managed:
        - source: salt://{{ slspath }}/conf/redis_server_default
        - mode: 644
        - template: jinja

/usr/local/yandex/start-pre.sh:
    file.managed:
        - template: jinja
        - require:
            - file: fix-aof-file
            - file: redis-fix-ip
        - source: salt://{{ slspath }}/conf/start-pre.sh
        - mode: 755
        - onchanges_in:
            - module: systemd-reload

include:
    - .aof
    - .start
    - .cleanup

extend:
{% if salt.mdb_redis.is_managed_cluster() %}
    redis-conf-reload:
        mdb_redis.set_options:
            - require:
                - service: redis-is-running

{%     if aof_enabled %}
    /usr/local/yandex/redis_rewrite_aof.py:
        file.managed:
            - require:
                - service: redis-is-running
{%     else %}
    /var/lib/redis/appendonly.aof:
        file.absent:
            - require:
                - service: redis-is-running
    /var/lib/redis/appendonlydir:
        file.absent:
            - require:
                - service: redis-is-running
{%     endif %}

{% endif %}

    redis-is-running:
        service.running:
            - enable: True
            - require:
                - file: redis-server-init
{% if salt.pillar.get('service-restart') %}
            - watch:
                - pkg: redis-pkgs
                - file: redis-server-init
                - file: redis-config-init
                - file: /etc/default/redis-server
{%     if redis.tls.enabled %}
                - file: /etc/redis/tls/server.crt
                - file: /etc/redis/tls/server.key
{%     endif %}
{% endif %}

/var/log/redis:
    file.directory:
        - dir_mode: 755
        - require:
            - pkg: redis-pkgs

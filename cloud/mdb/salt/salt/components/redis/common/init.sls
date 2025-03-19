/usr/local/yandex/redisctl:
    file.recurse:
        - source: salt://{{ slspath }}/conf/redisctl
        - file_mode: 755
        - dir_mode: 755
        - template: jinja
        - makedirs: True
        - include_empty: True
        - require:
            - file: redis-shared-metrics-common

{% if salt.mdb_redis.is_managed_cluster() %}
/usr/local/yandex/ensure_not_master.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/ensure_not_master.py
        - mode: 755
        - template: jinja
        - makedirs: True

/usr/local/yandex/ensure_no_primary.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/ensure_no_primary.sh
        - mode: 755
        - template: jinja
        - makedirs: True
        - require:
            - file: /usr/local/yandex/ensure_not_master.py

/usr/local/yandex/redis_userfault.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/redis_userfault.py
        - mode: 755
        - makedirs: True
        - template: jinja
        - require_in:
            - file: /etc/cron.d/mdb-redis-userfault

/etc/cron.d/mdb-redis-userfault:
    file.managed:
        - source: salt://{{ slspath }}/conf/redis_userfault.cron
        - mode: 755
        - makedirs: True
        - require:
            - file: /usr/local/yandex/redis_userfault.py
{% else %}
/etc/cron.d/mdb-redis-userfault:
    file.absent
{% endif %}

wd-script:
    file.absent:
        - name: /usr/bin/wd-service.sh

fix-aof-file:
    file.managed:
        - name: /usr/local/yandex/redis_fix_aof.sh
        - source: salt://{{ slspath }}/conf/redis_fix_aof.sh
        - mode: 755
        - makedirs: True
        - require:
            - pkg: redis-salt-module-requirements
            - pkg: redis-pkgs

redis-fix-ip:
    file.managed:
        - name: /usr/local/yandex/redis_fix_ip.py
        - source: salt://{{ slspath }}/conf/redis_fix_ip.py
        - template: jinja
        - mode: 755
        - makedirs: True
        - require:
            - pkg: redis-salt-module-requirements

{% if salt.dbaas.is_dataplane() %}
etc-fix-init:
    file.managed:
        - name: /lib/systemd/system/etc-fix.service
        - template: jinja
        - source: salt://{{ slspath }}/conf/etc-fix.service
        - mode: 644
        - require:
            - file: etc-fix
        - onchanges_in:
            - module: systemd-reload

etc-fix-service:
    service.enabled:
        - name: etc-fix
        - require:
            - file: etc-fix-init
{% endif %}

etc-fix:
    file.managed:
        - name: /usr/local/yandex/etc_fix.py
        - source: salt://{{ slspath }}/conf/etc_fix.py
        - mode: 755
        - makedirs: True
        - require:
            - pkg: redis-salt-module-requirements

/etc/logrotate.d/redis-server:
    file.managed:
        - source: salt://components/redis/common/conf/redis-server.logrotate
        - template: jinja
        - mode: 644
        - user: root
        - group: root

/etc/logrotate.d/redis-sentinel:
    file.managed:
        - source: salt://components/redis/common/conf/redis-sentinel.logrotate
        - template: jinja
        - mode: 644
        - user: root
        - group: root

include:
    - .ssl
    - .redispass
    - .info

extend:
{% if salt.pillar.get('data:monrun2', True) %}
    monitorhome-redispass:
        file.managed:
            - require:
                - pkg: juggler-pgks

    redis-shared-metrics:
        file.managed:
            - require:
                - pkg: juggler-pgks
{% endif %}

    redispass-file:
        file.managed:
            - name: {{ salt.mdb_redis.get_redispass_file() }}
            - require:
                - pkg: redis-pkgs

    redishome-redispass:
        file.managed:
            - require:
                - pkg: redis-pkgs

    redis-shared-metrics-common:
        file.managed:
            - require:
                - pkg: redis-pkgs

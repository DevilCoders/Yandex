/etc/init.d/redis-sentinel:
    file.absent

redis-sentinel-init:
    file.managed:
        - name: /lib/systemd/system/redis-sentinel.service
        - template: jinja
        - source: salt://{{ slspath }}/conf/redis-sentinel.service
        - mode: 644
        - require:
            - file: /usr/local/yandex/start-sentinel.sh
        - onchanges_in:
            - module: systemd-reload

/usr/local/yandex/start-sentinel.sh:
    file.managed:
        - template: jinja
        - require:
            - pkg: sentinel-pkgs
            - file: sentinel-config
            - file: /etc/default/redis-sentinel
        - source: salt://{{ slspath }}/conf/start-sentinel.sh
        - mode: 755
        - onchanges_in:
            - module: systemd-reload

/etc/default/redis-sentinel:
    file.managed:
        - source: salt://{{ slspath }}/conf/redis_sentinel_default
        - mode: 644
        - template: jinja

{% if not salt.pillar.get('data:redis:repair_sentinel', True) %}
{{ salt.mdb_redis.get_splitbrain_state_file() }}:
    file.absent
{% endif %}

include:
    - .service

extend:
    redis-sentinel:
        service.running:
            - enable: True
            - require:
                - pkg: sentinel-pkgs
            - watch:
                - file: sentinel-config
                - file: redis-sentinel-init

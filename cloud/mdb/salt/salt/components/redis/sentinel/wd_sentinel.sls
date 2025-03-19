{% set LOCK_CMD = 'flock -n /tmp/wd-sentinel.lock ' %}

repair-conf:
    file.managed:
        - name: /usr/local/yandex/repair.conf
        - template: jinja
        - source: salt://{{ slspath }}/conf/repair.conf
        - makedirs: True
        - mode: 640

repair-script:
    file.managed:
        - name: /usr/local/yandex/repair_sentinel.py
        - source: salt://{{ slspath }}/conf/repair_sentinel.py
        - template: jinja
        - makedirs: True
        - mode: 755
        - require:
            - file: repair-conf

watch-sentinel:
    file.managed:
        - name: /etc/cron.d/wd-redis-sentinel
        - contents: |
             PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
{% if salt.pillar.get('data:redis:repair_sentinel', True) %}
             * * * * * root {{ LOCK_CMD }}python /usr/local/yandex/repair_sentinel.py >>/var/log/redis/repair_sentinel.log 2>&1 || /bin/true
{% endif %}
        - mode: 644
        - require:
            - file: repair-script

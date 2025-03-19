rm-broken-tempfiles-script:
    file.managed:
        - name: /usr/local/yandex/rm_broken_tempfiles.sh
        - source: salt://{{ slspath }}/conf/rm_broken_tempfiles.sh
        - makedirs: True
        - mode: 755

watch-server:
    file.managed:
        - name: /etc/cron.d/wd-redis-server
        - contents: |
             PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
             * * * * * root chown redis:redis {{ salt.mdb_redis.get_redis_data_folder() }}
             * * * * * root /usr/local/yandex/rm_broken_tempfiles.sh >>/var/log/redis/rm_broken_tempfiles.log 2>&1 || /bin/true
        - mode: 644
        - require:
            - file: rm-broken-tempfiles-script
            - service: redis-is-running

redispass-file:
    file.managed:
        - name: {{ salt.mdb_redis.get_redispass_file() }}
        - source: salt://{{ slspath }}/conf/redispass
        - template: jinja
        - mode: 640
        - group: redis

redishome-redispass:
    file.managed:
        - name: {{ salt.mdb_redis.get_redispass_file('redis') }}
        - source: salt://{{ slspath }}/conf/redispass
        - template: jinja
        - mode: 600
        - user: redis
        - group: redis

monitorhome-redispass:
    file.managed:
        - name: {{ salt.mdb_redis.get_redispass_file('monitor') }}
        - source: salt://{{ slspath }}/conf/redispass
        - template: jinja
        - mode: 600
        - user: monitor
        - group: monitor

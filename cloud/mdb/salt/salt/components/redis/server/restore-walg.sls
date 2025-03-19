include:
    - .stop
    - components.redis.walg.restore-from

extend:
    stop-redis:
        service.dead:
            - require:
                - cmd: do-walg-restore

turn-appendonly-off:
    file.append:
        - name: {{ salt.mdb_redis.get_config_name() }}
        - text: 'appendonly no'
        - require:
            - service: stop-redis

start-redis:
    service.running:
        - name: redis-server
        - require:
            - file: turn-appendonly-off

turn-appendonly-on:
    mdb_redis.set_options:
        - options: {'appendonly': 'yes'}
        - require:
            - service: start-redis

start-rewrite-aof:
    mdb_redis.rewrite_aof:
        - require:
            - mdb_redis: turn-appendonly-on

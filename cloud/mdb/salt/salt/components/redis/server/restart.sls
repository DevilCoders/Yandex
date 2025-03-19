include:
    - .configs
    - .start
    - .stop

redis-config-drop:
    file.absent:
        - name: {{ salt.mdb_redis.get_config_name() }}
        - require:
            - service: stop-redis
        - require_in:
            - file: redis-config-init
            - file: redis-main-config
            - service: redis-is-running

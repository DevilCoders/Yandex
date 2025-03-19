{% set monitor_conf = {
        'slowlog-max-len': salt.pillar.get('data:redis:config:slowlog-max-len'),
    }
%}
redis-shared-metrics:
    file.managed:
        - name: {{ salt.mdb_metrics.get_redis_shared_metrics() }}
        - source: salt://{{ slspath }}/conf/info.json
        - template: jinja
        - defaults:
            conf: {{ monitor_conf }}
        - mode: 600
        - user: monitor
        - group: monitor


{% set redis_conf = {
        'redis_version_major': salt.mdb_redis.get_major_version(),
        'redis_port': salt.mdb_redis.get_redis_notls_port(),
        'sentinel_port': salt.mdb_redis.get_sentinel_notls_port(),
        'restarts' : salt.mdb_redis.get_redis_server_start_restart(),
        'timeout': salt.mdb_redis.get_redis_server_start_wait(),
        'sleep': salt.mdb_redis.get_redis_server_start_sleep(),
        'pidfile': salt.mdb_redis.get_redis_pidfile(),
        'repl_data': salt.mdb_redis.get_repl_data(),
        'redispass_files': salt.mdb_redis.get_redispass_files(),
        'rdb_full_path': salt.mdb_redis.get_rdb_fullpath(),
        'aof_full_path': salt.mdb_redis.get_aof_fullpath(),
        'aof_dir_path': salt.mdb_redis.get_aof_dirpath(),
        'wait_prestart_on_crash': salt.mdb_redis.get_wait_prestart_on_crash(),
        'infinite_wait_prestart_flag': salt.mdb_redis.get_infinite_wait_prestart_flag(),
    }
%}
redis-shared-metrics-common:
    file.managed:
        - name: {{ salt.mdb_redis.get_redis_shared_metrics_file() }}
        - source: salt://{{ slspath }}/conf/info.json
        - template: jinja
        - defaults:
            conf: {{ redis_conf|json }}
        - mode: 600
        - user: redis
        - group: redis

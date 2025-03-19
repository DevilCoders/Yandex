/etc/yandex/unified_agent/secrets/tvm:
  file.managed:
    - user: unified_agent
    - group: unified_agent
    - mode: 400
    - dir_mode: 750
    - makedirs: True
    - contents_pillar: 'unified_agent:tvm-client-secret'
/etc/yandex/unified_agent/conf.d/syslog-input.yml:
  file.managed:
    - user: unified_agent
    - group: unified_agent
    - mode: 644
    - dir_mode: 750
    - makedirs: True
    - contents: |
        main_thread_pool:
          threads: 4
        storages:
          - name: main
            plugin: fs
            config:
              directory: /var/lib/yandex/unified_agent/storage
              max_partition_size: 50gb
              data_retention:
                by_size: max
        routes:
          - input:
              plugin: syslog
              config:
                path: /tmp/unified_agent.sock
                max_message_size: 8192
                batch_size: 3000
            channel:
              pipe:
                #- filter:
                #    plugin: batch
                #    config:
                #      delimiter: ""
                #      flush_period: 600ms
                #      preserve_meta: timestamps
                #      limit:
                #        bytes: 500kb
                - filter:
                    plugin: compress
                    config:
                      codec: zstd
                      compression_quality: 3 # default 3
                - storage_ref:
                    name: main
                    flow_control:
                      inflight:
                        limit: 500mb
                - filter:
                    plugin: split_session
                    config:
                      # Число выходных сессий, между которыми нужно распределять данные.
                      sessions_count: 16  # обязательный
              output:
                #plugin: dev_null
                plugin: logbroker
                config:
                  endpoint: logbroker.yandex.net
                  port: 2135
                  topic:  /noc/nocdev/syslog
                  codec: zstd  # default gzip
                  compression_quality: 3 # default 3
                  tvm:
                    client_id: 2034573        # syslog
                    destination_id: 2001059   # logbroker
                    secret:
                      file: /etc/yandex/unified_agent/secrets/tvm
        services:
          # Тип сервиса logbroker_pqlib представляет экземпляр клиентской библиотеки logbroker.
          - type: logbroker_pqlib # обязательный

            # Имя экземпляра библиотеки.
            # По этому имени можно сослаться на конкретный экземпляр logbroker_pqlib,
            # указав его в свойстве pqlib_name в logbroker входе или выходе.
            # Если pqlib_name не указан, то используется значение logbroker_pqlib_default,
            # поэтому чтобы изменить конфигурацию logbroker_pqlib по умолчанию,
            # в name нужно указать это значение.
            name: logbroker_pqlib_default  # обязательный

            config:  # обязательный
              # Число потоков в основном пуле.
              threads_count: 12  # необязательный, по умолчанию 1

              # Число потоков, используемых для сжатия сообщений.
              compression_threads_count: 8  # необязательный, по умолчанию 2

              # Число потоков, используемых для взаимодействия с grpc.
              grpc_threads_count: 4  # необязательный, по умолчанию 1

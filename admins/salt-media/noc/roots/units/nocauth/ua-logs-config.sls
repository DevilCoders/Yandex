/etc/yandex/unified_agent/secrets/tvm:
  file.managed:
    - user: unified_agent
    - group: unified_agent
    - mode: 400
    - dir_mode: 750
    - makedirs: True
    - contents_pillar: 'sec:tvm'

/etc/yandex/unified_agent/conf.d/nocauth-logs.yml:
  file.managed:
    - user: unified_agent
    - group: unified_agent
    - mode: 644
    - dir_mode: 750
    - makedirs: True
    - contents: |
        routes:
          - input:
              plugin: journald
              config:
                state_directory: /var/tmp/unified-agent/journald
                match: >
                  _SYSTEMD_UNIT=nocauth-idm.service
                  _SYSTEMD_UNIT=nocauth-ssh.service
                  _SYSTEMD_UNIT=nocauth-tacacs.service
                import_fields:
                  unit: _SYSTEMD_UNIT
            channel:
              pipe:
                - filter:
                    plugin: split_session
                    config:
                      # Число выходных сессий, между которыми нужно распределять данные.
                      sessions_count: 12  # обязательный
              output:
                plugin: logbroker
                config:
                  endpoint: logbroker.yandex.net
                  port: 2135
                  topic:  /noc/nocdev/nocauth
                  codec: zstd  # default gzip
                  compression_quality: 3 # default 3
                  tvm:
                    client_id:      2024771   # nocauth
                    destination_id: 2001059   # logbroker
                    secret:
                      file: /etc/yandex/unified_agent/secrets/tvm
        services:
          - type: logbroker_pqlib
            name: logbroker_pqlib_default
            config:
              threads_count: 12
              compression_threads_count: 8
              grpc_threads_count: 4

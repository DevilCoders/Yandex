/etc/yandex/unified_agent/secrets/tvm:
  file.managed:
    - user: unified_agent
    - group: unified_agent
    - mode: 400
    - dir_mode: 750
    - makedirs: True
    - contents_pillar: 'unified_agent:tvm-client-secret'

/etc/yandex/unified_agent/conf.d/valve.yml:
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
                  _SYSTEMD_UNIT=valve.service
                import_fields:
                  unit: _SYSTEMD_UNIT
            channel:
              output:
                plugin: logbroker
                config:
                  endpoint: logbroker.yandex.net
                  port: 2135
                  topic:  /noc/nocdev/valve
                  codec: zstd  # default gzip
                  compression_quality: 3 # default 3
                  tvm:
                    client_id:      2035743   # nocdev/valve
                    destination_id: 2001059   # logbroker
                    secret:
                      file: /etc/yandex/unified_agent/secrets/tvm
/etc/yandex/unified_agent/conf.d/nginx.yml:
  file.managed:
    - user: unified_agent
    - group: unified_agent
    - mode: 644
    - dir_mode: 750
    - makedirs: True
    - contents: |
        routes:
          - input:
              plugin: syslog
              config:
                address: "[::1]:11514"
                max_message_size: 64kb
                format: rfc3164
                strip_tag_delimiter: true
            channel:
              output:
                plugin: logbroker
                config:
                  endpoint: logbroker.yandex.net
                  port: 2135
                  topic:  /noc/nocdev/nginx/valve
                  codec: zstd  # default gzip
                  compression_quality: 3 # default 3
                  tvm:
                    client_id:      2035743   # nocdev/valve
                    destination_id: 2001059   # logbroker
                    secret:
                      file: /etc/yandex/unified_agent/secrets/tvm

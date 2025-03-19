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
        routes:
          - input:
              plugin: syslog
              config:
                address: "[::1]:10514"
                max_message_size: 8192
                batch_size: 3000
            channel:
              output:
                plugin: logbroker
                config:
                  endpoint: logbroker.yandex.net
                  port: 2135
                  topic:  /noc/nocdev/gitlab
                  codec: zstd  # default gzip
                  compression_quality: 3 # default 3
                  tvm:
                    client_id: 2035507        # gitlab
                    destination_id: 2001059   # logbroker
                    secret:
                      file: /etc/yandex/unified_agent/secrets/tvm

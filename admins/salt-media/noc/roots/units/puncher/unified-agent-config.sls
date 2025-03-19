/etc/yandex/unified_agent/secrets/tvm:
  file.managed:
    - user: unified_agent
    - group: unified_agent
    - mode: 400
    - dir_mode: 750
    - makedirs: True
    - contents_pillar: 'puncher_secrets:tvm-secret'

/etc/yandex/unified_agent/conf.d/puncher.yml:
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
                  _SYSTEMD_UNIT=puncher-daemon.service
                  _SYSTEMD_UNIT=puncher-exportd.service
                  _SYSTEMD_UNIT=puncher-importd.service
                  _SYSTEMD_UNIT=puncher-macrosd.service
                  _SYSTEMD_UNIT=puncher-requestsd.service
                  _SYSTEMD_UNIT=puncher-rules-cauth.service
                  _SYSTEMD_UNIT=puncher-rulesd.service
                  _SYSTEMD_UNIT=puncher-st-watcher.service
                  _SYSTEMD_UNIT=puncher-usersd.service
                import_fields:
                  unit: _SYSTEMD_UNIT
            channel:
              output:
                plugin: logbroker
                config:
                  endpoint: logbroker.yandex.net
                  port: 2135
                  topic:  /noc/nocdev/puncher
                  codec: zstd  # default gzip
                  compression_quality: 3 # default 3
                  tvm:
                    client_id:      2027324   # puncher
                    destination_id: 2001059   # logbroker
                    secret:
                      file: /etc/yandex/unified_agent/secrets/tvm

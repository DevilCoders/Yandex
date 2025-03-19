{% set is_prod = grains["yandex-environment"] == "production" %}
/etc/yandex/unified_agent/secrets/tvm:
  file.managed:
    - user: unified_agent
    - group: unified_agent
    - mode: 400
    - dir_mode: 750
    - makedirs: True
    {% if is_prod %}
    - contents_pillar: 'sec:tvm-secret'
    {% else %}
    - contents: '!!!secret!!!'
    {% endif %}

/etc/yandex/unified_agent/conf.d/ck4k.yml:
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
                max_message_size: 64kb
            channel:
              output:
                {% if is_prod %}
                plugin: logbroker
                config:
                  endpoint: logbroker.yandex.net
                  port: 2135
                  topic:  /noc/nocdev/ck4k
                  codec: zstd  # default gzip
                  compression_quality: 3 # default 3
                  tvm:
                    client_id:      2023520   # ЦК/ЧК
                    destination_id: 2001059   # logbroker
                    secret:
                      file: /etc/yandex/unified_agent/secrets/tvm
                {% else %}
                plugin: dev_null
                {% endif %}

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
                {% if is_prod %}
                plugin: logbroker
                config:
                  endpoint: logbroker.yandex.net
                  port: 2135
                  topic:  /noc/nocdev/nginx/ck4k
                  codec: zstd  # default gzip
                  compression_quality: 3 # default 3
                  tvm:
                    client_id:      2023520   # ЦК/ЧК
                    destination_id: 2001059   # logbroker
                    secret:
                      file: /etc/yandex/unified_agent/secrets/tvm
                {% else %}
                plugin: dev_null
                {% endif %}

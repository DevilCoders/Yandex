/etc/yandex/unified_agent/secrets/tvm:
  file.managed:
    - user: unified_agent
    - group: unified_agent
    - mode: 400
    - dir_mode: 750
    - makedirs: True
    - contents_pillar: 'unified_agent:tvm-client-secret'

/etc/yandex/unified_agent/conf.d/http.yml:
  file.managed:
    - user: unified_agent
    - group: unified_agent
    - mode: 644
    - dir_mode: 750
    - makedirs: True
    - template: jinja
    - contents: |
        storages:
          - name: main_storage
            plugin: fs
            config:
              directory: /var/lib/yandex/unified_agent/main_storage
              max_partition_size: 1gb
              data_retention:
                by_size: max
        routes:
          - input:
              plugin: http
              config:
                host: localhost
                port: 2132
            channel:
              pipe:
                - storage_ref:
                    name: main_storage
              output:
                {%- if "-test-" in grains["fqdn"] %}
                plugin: debug
                id: my_output_id
                config:
                  delimiter: "\n"
                  file_name: /var/log/cvs-hook.log
                {%- else %}
                plugin: logbroker
                config:
                  endpoint: logbroker.yandex.net
                  port: 2135
                  topic:  /noc/nocdev/cvs
                  codec: zstd
                  compression_quality: 3
                  tvm:
                    client_id: 2035159        # cvs
                    destination_id: 2001059   # logbroker
                    secret:
                      file: /etc/yandex/unified_agent/secrets/tvm
                {%- endif %}


{% if "-test-" in grains["fqdn"] %}
/var/log/cvs-hook.log:  # ensure perms
  file.managed:
    - user: unified_agent
    - group: unified_agent
    - replace: False
{% endif %}


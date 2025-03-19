{{- with .Values.unifiedAgent }}
status:
  port: {{ required "You need to specify status port for unified agent" .statusPort }}

storages:
- name: main
  plugin: fs
  config:
    directory: /var/lib/unified_agent/data/storage
    max_partition_size: 10gb

routes:
- input:
    plugin: {{ .inputPlugin }}
    config:
      path: /var/run/unified_agent/unified_agent.sock
  channel:
    pipe:
      - storage_ref:
          name: main
      - filter:
          plugin: batch
          config:
            delimiter: "\n"
            flush_period: 100ms
            limit:
              bytes: 256kb
      - filter:
          plugin: assign
          config:
            session:
            - server: "{$host_name}"
    output:
      plugin: {{ required "You must specify output plugin for unified-agent" .outputPlugin }}
      config:
{{ required "You must specify output config for unified-agent" .outputConfig | toYaml | indent 12 }}
{{- end }}

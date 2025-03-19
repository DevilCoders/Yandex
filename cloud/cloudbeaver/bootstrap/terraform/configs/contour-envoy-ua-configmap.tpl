apiVersion: v1
kind: ConfigMap
metadata:
  name: contour-envoy-ua-config
data:
  config.yaml: |
    status:
      host: ""
      port: "16241"

    storages:
      - name: main
        plugin: fs
        config:
          directory: /var/lib/yandex/unified_agent/main
          max_partition_size: 100mb
          max_segment_size: 10mb

    channels:
      - name: cloud_monitoring
        channel:
          pipe:
            - storage_ref:
                name: main
          output:
            plugin: yc_metrics
            config:
              url: "${monitoring_url}"
              folder_id: "${folder_id}"
              iam:
                jwt:
                  file: "/etc/yandex/unified_agent/sa.json"
                  endpoint: "${iam_endpoint}"

    routes:
      - input:
          plugin: metrics_pull
          config:
            url: "${envoy_metrics_pull_url}"
            format:
              prometheus: {}
            namespace: ""
        channel:
          pipe:
            - filter:
                plugin: add_metric_labels
                config:
                  labels:
                    app: cloudbeaver-contour-envoy
                    namespace: default
          channel_ref:
            name: cloud_monitoring

      - input:
          plugin: agent_metrics
          config:
            namespace: ua
        channel:
          pipe:
            - filter:
                plugin: filter_metrics
                config:
                  match: "\"{scope=health}\""
          channel_ref:
            name: cloud_monitoring

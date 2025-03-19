unified-agent:
  metrics:
    mr-prober-agent-metrics:
      plugin: metrics
      input_inflight_limit_mb: 30
      storage_inflight_limit_mb: 30
      size_mb: 100
      config:
        project: cloud_mr_prober
        service: metrics
        port: 10050
        format:
          solomon_json: {}
        metric_name_label: metric

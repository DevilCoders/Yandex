unified-agent:
  metrics:
    mr-prober-api-metrics:
      plugin: metrics_pull
      input_inflight_limit_mb: 30
      storage_inflight_limit_mb: 30
      size_mb: 100
      config:
        project: cloud_mr_prober
        service: api
        url: "http://localhost:80/metrics"
        format:
          prometheus: {}

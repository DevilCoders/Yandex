mr_prober:
  versions:
    api: "1.5"
    creator: "1.23"
    agent: "1.11"

unified-agent:
  metrics:
    linux-metrics:
      input_inflight_limit_mb: 30
      storage_inflight_limit_mb: 30
      size_mb: 100
      config:
        project: cloud_mr_prober
    unified-agent-metrics:
      input_inflight_limit_mb: 30
      storage_inflight_limit_mb: 30
      size_mb: 100
      config:
        project: cloud_mr_prober

unified-agent:
  metrics:
    unified-agent-metrics:
      plugin: agent_metrics
      config:
        poll_period: 15s
        project: null
        service: unified-agent
        cluster: null
      storage: metrics_storage # optional field, metrics_storage will be used by default
    linux-metrics:
      plugin: linux_metrics
      config:
        poll_period: 15s
        project: null
        service: linux-metrics
        cluster: null
        resources:
          cpu: advanced
          memory: advanced
          network: advanced
          storage: advanced
          io: advanced
          kernel: basic
      storage: metrics_storage


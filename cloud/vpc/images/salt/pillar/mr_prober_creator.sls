unified-agent:
  metrics:
    mr-prober-creator-metrics:
      plugin: metrics
      config:
        project: cloud_mr_prober
        service: creator
        port: 10050
        format:
          solomon_json: {}
        metric_name_label: metric

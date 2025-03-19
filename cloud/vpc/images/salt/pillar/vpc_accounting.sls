unified-agent:
  pull_channel_port: 10001
  metrics:
    unified-agent-metrics:
      config:
        project: yc.vpc.accounting
    linux-metrics:
      config:
        project: yc.vpc.accounting

    vpc-accounting-static-metrics:
      plugin: metrics_pull
      config:
        project: yc.vpc.accounting
        service: vpc-accounting-sys
        url: http://localhost:15002/metrics 
        poll_period: 15s
        timeout: 10s
        retry_count: 1
        format:
          solomon_json: {}

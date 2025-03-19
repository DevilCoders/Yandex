{
  "/etc/solomon-agent/agent.conf": {
    "content": ${jsonencode(solomon_agent_conf)}
  },
  "/etc/yandex/statbox-push-client/push-client.yaml": {
    "content": ${jsonencode(push_client_conf)}
  },
  "/etc/metricsagent/metricsagent.yaml": {
    "content": ${jsonencode(metrics_agent_conf)}
  }
}
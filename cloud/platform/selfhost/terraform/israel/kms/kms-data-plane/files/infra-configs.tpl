{
  "/etc/solomon-agent/agent.conf": {
    "content": ${jsonencode(solomon_agent_conf)}
  },
  "/etc/solomon-agent-user-metrics/agent.conf": {
    "content": ${jsonencode(solomon_agent_user_metrics_conf)}
  }
}

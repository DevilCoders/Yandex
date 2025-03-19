{
  "/etc/yandex/statbox-push-client/push-client.yaml": {
    "content": ${jsonencode(push_client_conf)}
  },
  "/etc/solomon-agent/agent.conf": {
     "content": ${jsonencode(solomon_agent_conf)}
  }
}

{
  "/etc/yandex/statbox-push-client/push-client-yc-logbroker.yaml": {
    "content": ${jsonencode(push_client_yc_logbroker_conf)}
  },
  "/etc/solomon-agent/agent.conf": {
    "content": ${jsonencode(solomon_agent_conf)}
  },
  "/run/push-client/iam-key": {
    "content": ${jsonencode(push_client_iam_key)}
  }
}

{
  "/etc/yandex/statbox-push-client/billing-push-client-yc-logbroker.yaml": {
    "content": ${jsonencode(billing_push_client_yc_logbroker_conf)}
  },
  "/etc/yandex/statbox-push-client/push-client-yc-logbroker.yaml": {
    "content": ${jsonencode(push_client_yc_logbroker_conf)}
  },
  "/etc/solomon-agent/agent.conf": {
    "content": ${jsonencode(solomon_agent_conf)}
  },
  "/etc/solomon-agent-user-metrics/agent.conf": {
    "content": ${jsonencode(solomon_agent_user_metrics_conf)}
  }
}

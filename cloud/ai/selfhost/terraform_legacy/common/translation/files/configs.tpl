{
  "/etc/yandex/statbox-push-client/push-client.yaml": {
    "content": ${jsonencode(push_client_conf)}
  },
  "/etc/yandex/statbox-push-client/push-client-billing.yaml": {
    "content": ${jsonencode(push_client_billing_conf)}
  },
  "/etc/solomon-agent/agent.conf": {
    "content": ${jsonencode(solomon_agent_conf)}
  },
  "/etc/solomon-agent/translation-server.conf": {
    "content": ${jsonencode(solomon_agent_translation_server_conf)}
  },
  "/etc/logrotate.d/logrotate-app.conf": {
    "content": ${jsonencode(logrotate_app_conf)}
  }
}
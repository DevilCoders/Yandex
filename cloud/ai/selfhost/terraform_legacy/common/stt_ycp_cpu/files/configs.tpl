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
  "/etc/solomon-agent/solomon-agent-stt-server.conf": {
    "content": ${jsonencode(solomon_agent_stt_server_conf)}
  },
  "/etc/logrotate.d/logrotate-app.conf": {
    "content": ${jsonencode(logrotate_app_conf)}
  },
  "/etc/yc/ai/keys/data-sa/private.pem": {
    "content": ${jsonencode(data_sa_private_key)}
  },
  "/etc/yandex/cloud/ai/asr_remote_nodes.json": {
    "content": ${jsonencode(asr_remote_nodes)}
  }
}

{
  "/etc/yandex/statbox-push-client/push-client.yaml": {
    "content": ${jsonencode(push_client_conf)}
  },
  "/etc/solomon-agent/agent.conf": {
    "content": ${jsonencode(solomon_agent_conf)}
  },
  "/etc/solomon-agent/solomon-agent-tts-server.conf": {
    "content": ${jsonencode(solomon_agent_tts_server_conf)}
  },
  "/etc/logrotate.d/logrotate-app.conf": {
    "content": ${jsonencode(logrotate_app_conf)}
  }
}

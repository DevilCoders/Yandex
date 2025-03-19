{
  "/etc/yandex/statbox-push-client/push-client.yaml": {
    "content": ${jsonencode(push_client_conf)}
  },
  "/etc/api/configs/juggler-client/MANIFEST.json": {
    "content": ${jsonencode(juggler_bundle_manifest)}
  },
  "/etc/api/configs/juggler-client/platform-http-check.json": {
    "content": ${jsonencode(platform_http_check_json)}
  },
  "/etc/api/configs/juggler-client/juggler-client.conf": {
    "content": ${jsonencode(juggler_client_config)}
  },
  "/etc/metricsagent/metricsagent.yaml": {
    "content": ${jsonencode(metrics_agent_config)}
  }
}
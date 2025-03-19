{
  "/etc/fluent/fluent.conf": {
    "content": ${jsonencode(fluent_conf)}
  },
  "/etc/fluent/config.d/containers.input.conf": {
    "content": ${jsonencode(fluent_containers_input_conf)}
  },
  "/etc/fluent/config.d/system.input.conf": {
    "content": ${jsonencode(fluent_system_input_conf)}
  },
  "/etc/fluent/config.d/monitoring.conf": {
    "content": ${jsonencode(fluent_monitoring_conf)}
  },
  "/etc/fluent/config.d/output.conf": {
    "content": ${jsonencode(fluent_output_conf)}
  },
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
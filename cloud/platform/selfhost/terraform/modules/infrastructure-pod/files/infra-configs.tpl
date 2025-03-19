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
  "/etc/solomon-agent/agent.conf": {
    "content": ${jsonencode(solomon_agent_conf)}
  },
  "/etc/solomon-agent/als-prometheus.conf": {
    "content": ${jsonencode(solomon_agent_als_prom_conf)}
  },
  "/etc/solomon-agent/kubelet-prometheus.conf": {
    "content": ${jsonencode(solomon_agent_kubelet_prom_conf)}
  },
  "/etc/solomon-agent/envoy-prometheus.conf": {
    "content": ${jsonencode(solomon_agent_envoy_prom_conf)}
  },
  "/etc/solomon-agent/gateway-prometheus.conf": {
    "content": ${jsonencode(solomon_agent_gateway_prom_conf)}
  },
  "/etc/solomon-agent/system-plugin.conf": {
    "content": ${jsonencode(solomon_agent_system_plugin_conf)}
  },
  "/etc/api/configs/juggler-client/MANIFEST.json": {
    "content": ${jsonencode(juggler_bundle_manifest)}
  },
  "/etc/api/configs/juggler-client/platform-http-check.json": {
    "content": ${jsonencode(platform_http_check_json)}
  },
  "/etc/api/configs/juggler-client/juggler-client.conf": {
    "content": ${jsonencode(juggler_client_config)}
  }
}
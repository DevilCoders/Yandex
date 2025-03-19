{
  "/etc/api/envoy/envoy.yaml": {
    "content": ${jsonencode(envoy_config)}
  },
  "/etc/api/envoy/private-envoy.yaml": {
    "content": ${jsonencode(private_envoy_config)}
  },
  "/etc/api/envoy/ssl/certs/api-gateway.crt": {
    "content": ${jsonencode(envoy_cert)}
  },
  "/etc/api/envoy/ssl/private/api-gateway.key": {
    "content": ${jsonencode(envoy_key)}
  },
  "/etc/api/gateway/gateway.yaml": {
    "content": ${jsonencode(gateway_config)}
  },
  "/etc/api/configserver/configserver.yaml": {
    "content": ${jsonencode(configserver_config)}
  },
  "/etc/api/configserver/envoy-resources.yaml": {
    "content": ${jsonencode(envoy_resources)}
  },
  "/etc/api/configserver/gateway-services.yaml": {
    "content": ${jsonencode(gateway_services)}
  }
}

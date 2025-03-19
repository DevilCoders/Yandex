{
  "/etc/api/envoy/envoy.yaml": {
    "content": ${jsonencode(envoy_config)}
  },
  "/etc/api/envoy/ssl/certs/allCAs.pem": {
    "content": ${jsonencode(yandex_internal_root_ca)}
  },
  "/etc/api/envoy/private-envoy.yaml": {
    "content": ${jsonencode(private_envoy_config)}
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

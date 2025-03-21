{
  "/etc/api/configs/envoy/envoy.yaml": {
    "content": ${jsonencode(envoy_config)}
  },
  "/etc/api/configs/envoy/ssl/certs/allCAs.pem": {
    "content": ${jsonencode(yandex_internal_root_ca)}
  },
  "/etc/api/configs/private-envoy/private-envoy.yaml": {
    "content": ${jsonencode(private_envoy_config)}
  },
  "/etc/api/configs/private-envoy/ssl/certs/allCAs.pem": {
    "content": ${jsonencode(yandex_internal_root_ca)}
  },
  "/etc/api/configs/gateway/gateway.yaml": {
    "content": ${jsonencode(gateway_config)}
  },
  "/etc/api/configs/configserver/configserver.yaml": {
    "content": ${jsonencode(configserver_config)}
  },
  "/etc/api/configs/configserver/envoy-resources.yaml": {
    "content": ${jsonencode(envoy_resources)}
  },
  "/etc/api/configs/configserver/gateway-services.yaml": {
    "content": ${jsonencode(gateway_services)}
  }
}

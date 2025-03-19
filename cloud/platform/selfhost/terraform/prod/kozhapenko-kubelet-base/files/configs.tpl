{
  "/etc/api/configs/als/als.yaml": {
    "content": ${jsonencode(als_config)}
  },
  "/etc/api/configs/envoy/envoy.yaml": {
    "content": ${jsonencode(envoy_config)}
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
  },
  "/etc/api/configs/envoy/ssl/certs/api-gateway.crt": {
    "content": ${jsonencode(envoy_cert)}
  },
  "/etc/api/configs/envoy/ssl/private/api-gateway.key": {
    "content": ${jsonencode(envoy_key)}
  }
}
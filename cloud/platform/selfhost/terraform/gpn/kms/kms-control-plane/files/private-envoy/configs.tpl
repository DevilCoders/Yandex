{
  "/etc/api/envoy/private-envoy.yaml": {
    "content": ${jsonencode(private_envoy_config)}
  },
  "/etc/api/envoy/ssl/certs/api-gateway.crt": {
    "content": ${jsonencode(envoy_cert)}
  },
  "/etc/api/envoy/ssl/private/api-gateway.key": {
    "content": ${jsonencode(envoy_key)}
  }
}

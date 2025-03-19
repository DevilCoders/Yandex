{
  "/etc/l7/configs/als/als.yaml": {
    "content": ${jsonencode(als_config)}
  },
  "/etc/l7/configs/sds/sds.yaml": {
    "content": ${jsonencode(sds_config)}
  },
  "/etc/l7/configs/envoy/envoy.yaml": {
    "content": ${jsonencode(envoy_config)}
  },
  "/etc/l7/configs/envoy/ssl/certs/allCAs.pem": {
    "content": ${jsonencode(yandex_internal_root_ca)}
  },
  "/etc/l7/configs/envoy/ssl/certs/frontend.crt": {
    "content": ${jsonencode(envoy_cert)}
  },
  "/etc/l7/configs/envoy/ssl/private/frontend.key": {
    "content": ${jsonencode(envoy_key)}
  },
  "/etc/l7/configs/envoy/ssl/certs/xds-client.crt": {
    "content": ${jsonencode(envoy_xds_client_cert)}
  },
  "/etc/l7/configs/envoy/ssl/private/xds-client.key": {
    "content": ${jsonencode(envoy_xds_client_key)}
  },
  "/etc/l7/configs/envoyconfigdumper/envoyconfigdumper.yaml": {
    "content": ${jsonencode(envoyconfigdumper_config)}
  }
}

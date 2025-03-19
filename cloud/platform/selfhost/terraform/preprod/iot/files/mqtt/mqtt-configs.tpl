{
  "/etc/yc-iot/mqtt-server.yaml": {
    "content": ${jsonencode(mqtt_config)}
  },
  "/etc/yc-iot/mqtt/ssl/certs/server.crt": {
    "content": ${jsonencode(mqtt_cert)}
  },
  "/etc/yc-iot/mqtt/ssl/private/server.key": {
    "mode": "600",
    "content": ${jsonencode(mqtt_key)}
  },
  "${self_ping_certs}/sub_cert.pem": {
    "content": ${jsonencode(self_ping_sub_cert)}
  },
  "${self_ping_certs}/pub_cert.pem": {
    "content": ${jsonencode(self_ping_pub_cert)}
  },
  "/etc/yc-iot/mqtt/ssl/certs/allCAs.pem": {
    "content": ${jsonencode(yandex_internal_root_ca)}
  },
  "/etc/tvmtool/tvmtool.conf": {
    "content": ${jsonencode(tvmtool_config)}
  },
  "/etc/yandex/statbox-push-client/push-client.yaml": {
    "content": ${jsonencode(push_client_conf)}
  },
  "/etc/yandex/statbox-push-client/logging_secret": {
    "content": ${jsonencode(logging_secret_data)}
  }
}

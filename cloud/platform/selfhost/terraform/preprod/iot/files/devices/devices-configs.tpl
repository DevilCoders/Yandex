{
  "/etc/yc-iot/devices/ssl/certs/server.crt": {
    "content": ${jsonencode(devices_cert)}
  },
  "/etc/yandex/statbox-push-client/push-client.yaml": {
    "content": ${jsonencode(push_client_conf)}
  },
  "/etc/log4j2.yaml": {
    "content": ${jsonencode(log4j_conf)}
  }
}

{
  "/etc/yandex/statbox-push-client/push-client.yaml": {
    "content": ${jsonencode(push_client_conf)}
  },
  "/etc/yandex/statbox-push-client/push-client-yc-logbroker.yaml": {
    "content": ${jsonencode(push_client_yc_logbroker_conf)}
  }
}
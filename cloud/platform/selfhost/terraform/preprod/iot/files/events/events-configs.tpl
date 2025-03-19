{
  "/etc/yc-iot/events_broker.yaml": {
    "content": ${jsonencode(events_config)}
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

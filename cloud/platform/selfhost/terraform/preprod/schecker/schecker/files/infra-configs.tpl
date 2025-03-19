{
  "/etc/push-client/push-client.yaml": {
    "content": ${jsonencode(push_client_conf)}
  },
  "/run/push-client/iam-key": {
    "content": ${push_client_iam_key}
  }
}

{
  "/etc/yandex/statbox-push-client/push-client.yaml": {
    "content": ${jsonencode(push_client_conf)}
  },
  "/etc/yandex/statbox-push-client/billing-push-client.yaml": {
    "content": ${jsonencode(billing_push_client_conf)}
  },
  "/etc/yandex/statbox-push-client/oauth-token": {
    "content": ${jsonencode(push_client_oauth_token)}
  },
  "/etc/solomon-agent/agent.conf": {
    "content": ${jsonencode(solomon_agent_conf)}
  },
  "/etc/kms/certificate.pem": {
    "content": ${jsonencode(kms_certificate_pem)}
  }
}
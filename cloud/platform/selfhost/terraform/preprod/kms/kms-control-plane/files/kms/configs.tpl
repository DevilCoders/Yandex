{
  "/etc/kms/application.yaml": {
    "content": ${jsonencode(application_yaml)}
  },
  "/etc/kms/ssl/certs/allCAs.pem": {
    "content": ${jsonencode(yandex_internal_root_ca)}
  }
}
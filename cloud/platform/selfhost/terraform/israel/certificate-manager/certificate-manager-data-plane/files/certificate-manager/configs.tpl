{
  "/etc/certificate-manager/application.yaml": {
    "content": ${jsonencode(application_yaml)}
  },
  "/etc/certificate-manager/ssl/certs/allCAs.pem": {
    "content": ${jsonencode(yandex_internal_root_ca)}
  }
}

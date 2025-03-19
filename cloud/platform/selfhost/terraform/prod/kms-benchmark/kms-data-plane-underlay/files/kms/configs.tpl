{
  "/etc/kms/application.yaml": {
    "content": ${jsonencode(application_yaml)}
  },
  "/etc/kms/certificate.pem": {
    "content": ${jsonencode(kms_certificate_pem)}
  }
}

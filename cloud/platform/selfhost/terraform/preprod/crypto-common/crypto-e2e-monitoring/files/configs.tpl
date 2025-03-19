{
  "/etc/e2e-monitoring/application.yaml": {
    "content": ${jsonencode(application_yaml)}
  },
  "/etc/e2e-monitoring/certificate.pem": {
    "content": ${jsonencode(e2e_monitoring_certificate_pem)}
  }
}

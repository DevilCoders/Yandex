{
  "/etc/jaegermeister/jaegermeister.yaml": {
    "content": ${jsonencode(jaegermeister_config)}
  },
  "/etc/jaegermeister/ssl/certs/allCAs.pem": {
    "content": ${jsonencode(yandex_internal_root_ca)}
  }
}
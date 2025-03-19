module "ig" {
  source = "../../../../modules/l7-envoy-ig"

  #######################
  # Names
  name                   = var.name
  conductor_group_suffix = var.name
  ma_name                = var.name
  tracing_service        = var.tracing_service

  #######################
  # IDs
  alb_lb_id   = var.alb_lb_id
  image_id    = "c2pkn3g9i208rolan98c"
  ig_sa       = var.ig_sa
  instance_sa = var.instance_sa

  #######################
  # Env
  env    = "testing"
  env2   = "testing"
  domain = "cloud-testing.yandex.net"

  #######################
  # Secrets
  server_cert_pem = var.server_cert_pem
  server_cert_key = var.server_cert_key

  client_cert_pem = file("${path.module}/client.pem")
  client_cert_key = file("${path.module}/client_key.json")

  solomon_token = file("${path.module}/solomon-token.json")

  #######################
  # Endpoints
  kms_endpoint          = "kms.cloud-testing.yandex.net"
  alb_endpoint          = "not-used.localhost:666"
  cert_manager_endpoint = "not-used.localhost:666"

  xds_endpoints = [
    "2a02:6b8:c0e:2c0:0:fc1a:0:30",
  ]
  xds_sni = "xds.ycp.cloud-preprod.yandex.net"

  #######################
  # Network
  network_id = "a198qjop82n721k01hh7"
  subnet_ids = [
    "emafm3s681n6qbcr5u6a",
  ]
  zones = [
    "ru-central1-a",
  ]

  #######################
  # Misc.
  sds_log_level     = "DEBUG"
  push_client_ident = "yc_api@testing"
}

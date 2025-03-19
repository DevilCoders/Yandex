module "ig" {
  source = "../../../../modules/l7-envoy-ig"

  #######################
  # Names
  name                   = var.name
  conductor_group_suffix = var.conductor_group_suffix
  ma_name                = var.name
  tracing_service        = var.tracing_service

  #######################
  # IDs
  alb_lb_id   = var.alb_lb_id
  image_id    = "fdvfv9l627a3d7rj574b" # CLOUD-53121, 09.09.2020
  ig_sa       = var.ig_sa
  instance_sa = var.instance_sa

  #######################
  # Env
  env    = "preprod"
  env2   = "pre-prod"
  domain = "cloud-preprod.yandex.net"

  #######################
  # Secrets

  solomon_token = file("${path.module}/solomon-token.json")

  #######################
  # Endpoints
  kms_endpoint          = "kms.cloud-preprod.yandex.net"

  # ALS
  remote_als_addr = "als.ycp.cloud-preprod.yandex.net"

  xds_endpoints = [
    "2a02:6b8:c0e:501:0:f806:0:30",
    "2a02:6b8:c02:901:0:f806:0:30",
    "2a02:6b8:c03:501:0:f806:0:30",
  ]
  xds_sni = "xds.ycp.cloud-preprod.yandex.net"

  #######################
  # Network
  network_id = "c64hinjaf2rjjpsit25s"
  subnet_ids = [
    "bucqe7hncapq5rrr6os3",
    "blto554kooj9q9qbs2l4",
    "fo2doum42mfhj1nea6sp",
  ]
  zones = [
    "ru-central1-a",
    "ru-central1-b",
    "ru-central1-c",
  ]

  #######################
  # Misc.
  xds_auth = true

  push_client_ident = "yc_api@preprod"
  size = var.fixed_size
  has_ipv4 = var.has_ipv4
  enable_tracing = var.enable_tracing
  files = {
    "/etc/l7/configs/sds/sds.yaml" = file("${path.module}/../../common/sds.yaml")

    "/etc/l7/configs/envoy/ssl/certs/frontend.crt"   = var.server_cert_pem
    "/etc/l7/configs/envoy/ssl/private/frontend.key" = var.server_cert_key

    "/etc/l7/configs/envoy/ssl/certs/xds-client.crt"   = file("${path.module}/client.pem")
    "/etc/l7/configs/envoy/ssl/private/xds-client.key" = file("${path.module}/client_key.json")

    "/etc/jaeger-agent/jaeger-agent-config.yaml" = file("${path.module}/../../common/jaeger-agent.yaml")
  }
}

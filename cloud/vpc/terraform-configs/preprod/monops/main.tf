// Staging installation of monops
// Viewer service account for production installation of monops

module "monops_web" {
  source = "../../modules/tools/monops/web/"

  // yc-tools/vpc on preprod
  folder_id = "aoech37sn3h9q93u2sjf"
  ycp_profile = var.ycp_profile
  yc_endpoint = var.yc_endpoint
  cr_endpoint = var.cr_endpoint
  hc_network_ipv6 = var.hc_network_ipv6
  environment = var.environment

  oauth_endpoint = "https://auth-preprod.cloud.yandex.ru/oauth/token"
  ss_endpoint = "grpc://ss.private-api.cloud-preprod.yandex.net:8655"
  iam_endpoint = "grpc://iam.private-api.cloud-preprod.yandex.net:4283"
  as_host = "as.private-api.cloud-preprod.yandex.net"

  auth_environments_bypass = ["preprod"]
  auth_environments_proxy = ["testing"]

  monops_host = "monops-preprod.cloud.yandex.net"
  monops_ipv6_address = "2a0d:d6c0:0:ff1c::1214"

  monops_zones = ["ru-central1-c"]
  monops_network_id = "c64u8jrv3thvflqsinmh"
  monops_network_subnet_ids = {
    ru-central1-a = "bucfs8b7ub3nbpu27s0s"
    ru-central1-b = "blt51409ua25dcueqlvi"
    ru-central1-c = "fo2de2qbj1r9jljioijh"
  }

  dns_zone_id = "aet3tvjhmb7f6jqpc3b8"
  dns_zone = "monops.vpc.cloud-preprod.yandex.net"
}

module "monops_sa" {
  source = "../../modules/tools/monops/viewer_sa/"

  // yc-tools/vpc on preprod
  folder_id = "aoech37sn3h9q93u2sjf"
}

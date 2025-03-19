// Production installation of monops
// No viewer sa here as bypass authorization should be used

module "monops_web" {
  source = "../../modules/tools/monops/web/"

  // yc-tools/vpc on prod
  folder_id = "b1gutrms0n1dtticj582"
  ycp_profile = var.ycp_profile
  yc_endpoint = var.yc_endpoint
  cr_endpoint = var.cr_endpoint
  hc_network_ipv6 = var.hc_network_ipv6
  environment = var.environment

  oauth_endpoint = "https://auth.cloud.yandex.ru/oauth/token"
  ss_endpoint = "grpc://ss.private-api.cloud.yandex.net:8655"
  iam_endpoint = "grpc://iam.private-api.cloud.yandex.net:4283"
  as_host = "as.private-api.cloud.yandex.net"

  auth_environments_bypass = ["prod"]
  auth_environments_proxy = ["preprod", "testing"]

  monops_host = "monops.cloud.yandex.net"
  monops_ipv6_address = "2a0d:d6c0:0:1c::1214"

  monops_network_id = "enp52ls83feut9ct9pqg"
  monops_network_subnet_ids = {
    ru-central1-a = "e9bigio0vhk246euavkb"
    ru-central1-b = "e2l36n63vhg7vg6b9j8r"
    ru-central1-c = "b0crolik07ik4hsqg543"
  }

  dns_zone_id = "dnsbr1gljs48aovvru12"
  dns_zone = "monops.vpc.cloud.yandex.net"
}

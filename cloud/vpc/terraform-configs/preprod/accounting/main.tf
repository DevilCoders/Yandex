// https://st.yandex-team.ru/CLOUD-74169
// https://racktables.yandex-team.ru/index.php?page=services&tab=projects&project_name=_CLOUD_VPC_ACCOUNTING_PREPROD_NETS_
locals {
  vpc_accounting_preprod_nets = parseint("fc5c", 16)
}

module "accounting" {
  source = "../../modules/accounting/"

  // VLA - http://netbox.cloud.yandex.net/ipam/prefixes/391/
  // SAS - http://netbox.cloud.yandex.net/ipam/prefixes/392/
  // MYT - http://netbox.cloud.yandex.net/ipam/prefixes/393/
  accounting_network_ipv6_cidrs = {
    ru-central1-a = cidrsubnet("2a02:6b8:c0e:501::/64", 32, local.vpc_accounting_preprod_nets)
    ru-central1-b = cidrsubnet("2a02:6b8:c02:901::/64", 32, local.vpc_accounting_preprod_nets)
    ru-central1-c = cidrsubnet("2a02:6b8:c03:501::/64", 32, local.vpc_accounting_preprod_nets)
  }

  // https://st.yandex-team.ru/CLOUD-74766
  dns_zone    = "vpc-accounting.cloud-preprod.yandex.net"
  dns_zone_id = "aet5fv76lu654u6b8605"

  log_level = "info"

  ycp_profile                   = var.ycp_profile
  yc_endpoint                   = var.yc_endpoint
  environment                   = var.environment
  logbroker_accounting_database = var.logbroker_database
  logbroker_billing_database    = var.logbroker_database
  solomon_url                   = "https://solomon.cloud-preprod.yandex-team.ru"
  logbroker_endpoint            = var.logbroker_endpoint
  hc_network_ipv6               = var.hc_network_ipv6
  test_instances_count          = 1
  accounting_instances_count    = 1
  accounting_image              = "fdvehcgstvjhh8921vvg"
}

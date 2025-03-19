// https://st.yandex-team.ru/CLOUD-74169
// https://racktables.yandex-team.ru/index.php?page=services&tab=projects&project_name=_CLOUD_VPC_ACCOUNTING_PROD_NETS_
locals {
  vpc_accounting_prod_nets = parseint("f874", 16)
}

module "accounting" {
  source = "../../modules/accounting/"


  // VLA - http://netbox.cloud.yandex.net/ipam/prefixes/388/
  // SAS - http://netbox.cloud.yandex.net/ipam/prefixes/389/
  // MYT - http://netbox.cloud.yandex.net/ipam/prefixes/390/
  accounting_network_ipv6_cidrs = {
    ru-central1-a = cidrsubnet("2a02:6b8:c0e:500::/64", 32, local.vpc_accounting_prod_nets)
    ru-central1-b = cidrsubnet("2a02:6b8:c02:900::/64", 32, local.vpc_accounting_prod_nets)
    ru-central1-c = cidrsubnet("2a02:6b8:c03:500::/64", 32, local.vpc_accounting_prod_nets)
  }

  // https://st.yandex-team.ru/CLOUD-74767
  dns_zone    = "vpc-accounting.cloud.yandex.net"
  dns_zone_id = "dns2qhm3v2le0tq7hb0e"

  ycp_profile                   = var.ycp_profile
  yc_endpoint                   = var.yc_endpoint
  environment                   = var.environment
  logbroker_accounting_database = var.logbroker_database
  logbroker_billing_database    = var.logbroker_database
  solomon_url                   = "https://solomon.cloud.yandex-team.ru"
  logbroker_endpoint            = var.logbroker_endpoint
  hc_network_ipv6               = var.hc_network_ipv6
  accounting_instances_count    = 20
  test_instances_count          = 15
  accounting_image              = "fd812jn792dou8jobi07"
}

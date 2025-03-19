// https://st.yandex-team.ru/CLOUD-105767
// https://racktables.yandex-team.ru/index.php?page=services&tab=projects&project_name=_CLOUD_IL_VPC_ACCOUNTING_NETS_
locals {
  vpc_accounting_il_nets = parseint("90000046", 16)
}

module "accounting" {
  source = "../../modules/accounting/"

  accounting_network_subnet_zones = var.availability_zones

  accounting_network_ipv4_cidrs = {
    il1-a = "172.16.0.0/16"
  }

  // IL1-A - https://netbox.cloud.yandex.net/ipam/prefixes/3516/
  accounting_network_ipv6_cidrs = {
    il1-a = cidrsubnet("2a11:f740:1::/64", 32, local.vpc_accounting_il_nets)
  }

  // https://st.yandex-team.ru/CLOUD-74767
  dns_zone    = "vpc-accounting.yandexcloud.co.il"
  dns_zone_id = "yc.vpc.accounting.dns-zone"

  vm_platform_id                = "standard-v3"
  ycp_profile                   = var.ycp_profile
  yc_endpoint                   = var.yc_endpoint
  environment                   = var.environment
  logbroker_accounting_database = "/israel_global/yc.vpc.accounting/p7fr7ghj1clg7if7k"
  logbroker_billing_database    = "/israel_global/yc.billing.service-cloud/n4vtkakfmj2edce3a"
  logbroker_billing_topic       = "/services/billing-sdn-traffic-nfc"
  logbroker_loadbalancer_topic  = "/services/billing-lb-traffic-nfc"
  logbroker_accounting_topic    = "/accounting-topic"
  solomon_url                   = "https://solomon.yandexcloud.co.il"
  logbroker_endpoint            = var.logbroker_endpoint
  hc_network_ipv6               = var.hc_network_ipv6
  accounting_instances_count    = 1
  test_instances_count          = 0
  accounting_image              = "alkljs28hlb9h1kr2io5"
}

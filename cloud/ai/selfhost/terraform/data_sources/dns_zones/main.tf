/*
 * data_sources/dns_zones - contains exported datasources data in suitable format
 * for usage with standard_service && standard_instance_group modules
 * each output entry represent everything needed for configuration of ONE dns_zone
 *
 * each output variable should have format
 * {
   TBD
 * }
 */

locals {
  is_prod = var.environment == "preprod" ? false : true
}

data "yandex_dns_zone" "preprod_ds_private_dns_zone" {
  count       = local.is_prod ? 0 : 1
  dns_zone_id = "aet1g1q9s5hkll8le5c2"
}

data "yandex_dns_zone" "prod_ds_private_dns_zone" {
  count       = local.is_prod ? 1 : 0
  dns_zone_id = "dns8glno30d77stabla9"
}

data "yandex_dns_zone" "prod_ds_public_dns_zone" {
  count       = local.is_prod ? 1 : 0
  dns_zone_id = "dnsfm932b3j1s17ihkks"
}

locals {
  ds_private_dns_zone = {
    preprod = local.is_prod ? null : data.yandex_dns_zone.preprod_ds_private_dns_zone[0]
    staging = local.is_prod ? data.yandex_dns_zone.prod_ds_private_dns_zone[0] : null
    prod    = local.is_prod ? data.yandex_dns_zone.prod_ds_private_dns_zone[0] : null
  }

  ds_public_dns_zone = {
    preprod = null
    staging = null
    prod    = local.is_prod ? data.yandex_dns_zone.prod_ds_public_dns_zone[0] : null
  }
}

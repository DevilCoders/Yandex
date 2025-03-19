module "metadata" {
  source = "../../modules/metadata"

  for_each = local.services

  #service_name   = each.key
  service_name   = each.value.service_name
  packet_name    = each.value.packet_name
  packet_version = each.value.packet_version

  env_name = local.env_name
  domain   = var.domain

  self_dns_api_host = var.selfdns_api_host
  selfdns_secret    = var.selfdns_secret

  lockbox_api_host    = var.lockbox_api_host
  sa_consumer_secret  = var.sa_consumer_secret
  sa_consumer_version = var.sa_consumer_version

  searchmap = local.searchmap

  zk_config = local.zk_config
}

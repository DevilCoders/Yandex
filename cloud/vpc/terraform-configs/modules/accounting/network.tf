resource "ycp_vpc_network" "network" {
  // Well-known id of accounting folder in "yc.vpc.accounting" cloud. See https://bb.yandex-team.ru/projects/CLOUD/repos/bootstrap-templates/browse/terraform/*/vpc.tf
  folder_id   = var.folder_id

  name        = "accounting-network"
  description = "Network for accounting cluster"
}

resource "ycp_vpc_subnet" "subnets" {
  for_each       = toset(var.accounting_network_subnet_zones)

  folder_id      = var.folder_id
  network_id     = ycp_vpc_network.network.id

  name           = format("accounting-subnet-%s", each.key)
  zone_id        = each.key


  v4_cidr_blocks = [var.accounting_network_ipv4_cidrs[each.key]]
  v6_cidr_blocks = [var.accounting_network_ipv6_cidrs[each.key]]

  extra_params {
    export_rts   = ["65533:666"]
    hbf_enabled  = var.accounting_network_hbf_enabled
    import_rts   = ["65533:776"]
    rpf_enabled  = false
  }
}


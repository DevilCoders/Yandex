resource "ycp_vpc_network" "network" {
  folder_id   = var.folder_id

  name        = "${var.prefix}-network"
  description = "Network for cluster"
}

resource "ycp_vpc_subnet" "subnets" {
  for_each       = toset(var.monitoring_network_zones)

  folder_id      = var.folder_id
  network_id     = ycp_vpc_network.network.id

  name           = format("%s-network-%s", var.prefix, each.key)
  zone_id        = each.key

  v4_cidr_blocks = [cidrsubnet(var.monitoring_network_ipv4_cidrs[each.key], 8, 0)]
  v6_cidr_blocks = [cidrsubnet(var.monitoring_network_ipv6_cidrs[each.key], 8, 0)]
  extra_params {
    import_rts   = ["65533:776"]
    export_rts   = ["65533:666"]
    hbf_enabled  = lookup(local.hbf_environments, var.ycp_profile, true)
    rpf_enabled  = false
  }
}

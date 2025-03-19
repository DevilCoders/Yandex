resource "ycp_vpc_subnet" "agent_subnets" {
  for_each       = toset(var.monitoring_network_zones)

  folder_id      = var.folder_id
  network_id     = ycp_vpc_network.network.id

  name           = format("%s-agent-subnet-%s", var.prefix, each.key)
  zone_id        = each.key
  route_table_id = ycp_vpc_route_table.nat_routes.id

  v4_cidr_blocks = [cidrsubnet(var.monitoring_network_ipv4_cidrs[each.key], 8, 1)]
  v6_cidr_blocks = [cidrsubnet(var.monitoring_network_ipv6_cidrs[each.key], 8, 1)]
  extra_params {
    import_rts   = ["65533:776"]
    export_rts   = ["65533:666"]
    hbf_enabled  = lookup(local.hbf_environments, var.ycp_profile, true)
    rpf_enabled  = false
  }
}

resource "ycp_vpc_subnet" "nat_instance_subnets" {
  for_each       = toset(var.monitoring_network_zones)

  folder_id      = var.folder_id
  network_id     = ycp_vpc_network.network.id

  name           = format("%s-nat-subnet-%s", var.prefix, each.key)
  zone_id        = each.key

  v4_cidr_blocks = [cidrsubnet(var.monitoring_network_ipv4_cidrs[each.key], 8, 2)]
  v6_cidr_blocks = [cidrsubnet(var.monitoring_network_ipv6_cidrs[each.key], 8, 2)]
  extra_params {
    import_rts   = ["65533:776"]
    export_rts   = ["65533:666"]
    hbf_enabled  = lookup(local.hbf_environments, var.ycp_profile, true)
    rpf_enabled  = false
  }
}

resource "ycp_vpc_route_table" "nat_routes" {
  folder_id      = var.folder_id
  network_id     = ycp_vpc_network.network.id

  dynamic "static_route" {
    for_each = var.static_routes

    content {
      destination_prefix = replace(replace(
        static_route.value.destination_prefix,
          "target0", ycp_compute_instance.target[0].network_interface[0].primary_v4_address[0].one_to_one_nat[0].address),
            "target1", ycp_compute_instance.target[1].network_interface[0].primary_v4_address[0].one_to_one_nat[0].address)
      next_hop_address = lookup(ycp_compute_instance.nat_instance, static_route.value.next_hop).network_interface[0].primary_v4_address[0].address
    }
  }
}

resource "ycp_vpc_network" "target_network" {
  folder_id   = var.folder_id

  name        = "${var.prefix}-target-network"
  description = "Network for cluster targets"
}

resource "ycp_vpc_subnet" "target_subnets" {
  for_each       = toset(var.monitoring_network_zones)

  folder_id      = var.folder_id
  network_id     = ycp_vpc_network.target_network.id

  name           = format("%s-target-subnet-%s", var.prefix, each.key)
  zone_id        = each.key

  v4_cidr_blocks = [cidrsubnet(var.monitoring_network_ipv4_cidrs[each.key], 8, 3)]
  v6_cidr_blocks = [cidrsubnet(var.monitoring_network_ipv6_cidrs[each.key], 8, 3)]
  extra_params {
    import_rts   = ["65533:776"]
    export_rts   = ["65533:666"]
    hbf_enabled  = lookup(local.hbf_environments, var.ycp_profile, true)
    rpf_enabled  = false
  }
}

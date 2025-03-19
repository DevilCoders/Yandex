locals {
  router_instance_host      = 10
  worker_instance_host_base = 20

  // An address of pseudo-pod assigned as eth0:x
  logical_host_number = 3

  router_instance_ipv4_addrs = [
    for index in range(2) : cidrhost(
      var.monitoring_network_ipv4_cidrs[var.router_instance_zone],
    local.router_instance_host + index)
  ]
  router_instance_ipv6_addrs = [
    for index in range(2) : cidrhost(
      cidrsubnet(var.v6_cidr, 32, var.subnet_zone_bits[var.router_instance_zone]),
    local.router_instance_host + index)
  ]

  v6_zone_cidrs = {
    for index, zone in tolist(toset(var.worker_zones)) :
    zone => cidrsubnet(var.v6_cidr, 32, index)
  }

  worker_cidrs = tolist([
    for index, zone in var.worker_zones : {
      v4_inner_cidr     = cidrsubnet(var.worker_v4_cidr, 8, index)
      v4_inner_address  = cidrhost(cidrsubnet(var.worker_v4_cidr, 8, index), local.logical_host_number)
      v4_default_router = cidrhost(var.monitoring_network_ipv4_cidrs[zone], 1)
      v4_address        = cidrhost(var.monitoring_network_ipv4_cidrs[zone], local.worker_instance_host_base + index)

      v6_inner_cidr     = cidrsubnet(var.worker_v6_cidr, 32, index)
      v6_inner_address  = cidrhost(cidrsubnet(var.worker_v6_cidr, 32, index), local.logical_host_number)
      v6_default_router = cidrhost(cidrsubnet(var.v6_cidr, 32, var.subnet_zone_bits[zone]), 1)
      v6_address        = cidrhost(cidrsubnet(var.v6_cidr, 32, var.subnet_zone_bits[zone]), local.worker_instance_host_base + index)
    }
  ])
}

resource "ycp_vpc_network" "k8s" {
  folder_id = var.folder_id

  name        = "${var.prefix}-network"
  description = "Network for intra-vpc tests"
}

resource "ycp_vpc_route_table" "k8s" {
  folder_id  = var.folder_id
  network_id = ycp_vpc_network.k8s.id

  // K8s routes. These should be in SystemRT, but it cannot be created via terraform because it
  // needs created network first (and should be plugged into it), so we concatenate routes manually
  dynamic "static_route" {
    for_each = local.worker_cidrs
    content {
      destination_prefix = static_route.value.v4_inner_cidr
      next_hop_address   = static_route.value.v4_address
    }
  }

  dynamic "static_route" {
    for_each = local.worker_cidrs
    content {
      destination_prefix = static_route.value.v6_inner_cidr
      next_hop_address   = static_route.value.v6_address
    }
  }

  // Aggregate routes for remote network
  static_route {
    destination_prefix = var.routable_v4_cidr
    next_hop_address   = local.router_instance_ipv4_addrs[var.use_local_compute_node ? 0 : 1]
  }

  static_route {
    destination_prefix = var.routable_v6_cidr
    next_hop_address   = local.router_instance_ipv6_addrs[var.use_local_compute_node ? 0 : 1]
  }
}

resource "ycp_vpc_subnet" "k8s" {
  for_each = var.subnet_zone_bits

  folder_id      = var.folder_id
  network_id     = ycp_vpc_network.k8s.id
  route_table_id = ycp_vpc_route_table.k8s.id

  name    = format("%s-subnet-%s", var.prefix, each.key)
  zone_id = each.key

  // NOTE: v4/v6 CIDRs should be able to be safely reused since all tests are using only
  // inner CIDRs and we have DMZ (no import/export rts). This way the test is even more fun ;)
  v4_cidr_blocks = [var.monitoring_network_ipv4_cidrs[each.key]]
  v6_cidr_blocks = [cidrsubnet(var.v6_cidr, 32, var.subnet_zone_bits[each.key])]
  extra_params {
    rpf_enabled = false
  }
}

output "subnets" {
  value = ycp_vpc_subnet.k8s
}

output "router_instance_ipv4_addrs" {
  value = local.router_instance_ipv4_addrs
}

output "router_instance_ipv6_addrs" {
  value = local.router_instance_ipv6_addrs
}

output "worker_cidrs" {
  value = local.worker_cidrs
}
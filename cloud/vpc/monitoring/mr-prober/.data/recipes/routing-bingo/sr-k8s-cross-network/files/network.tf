locals {
  ri_compute_node = element([
    for compute_node in var.compute_nodes :
  compute_node if compute_node.has_agent == var.use_local_compute_node], 0)

  v6_cidr = "fc00::/32"

  v4_inner_cidr       = "172.20.0.0/15"
  v4_left_inner_cidr  = "172.20.0.0/16"
  v4_right_inner_cidr = "172.21.0.0/16"

  // NOTE: fd00::/80 is reserved for docker
  v6_inner_cidr       = "fd01::/16"
  v6_left_inner_cidr  = "fd01::/32"
  v6_right_inner_cidr = "fd01:1::/32"
}

module "left" {
  source = "./k8s_network"

  folder_id                     = var.folder_id
  prefix                        = "${var.prefix}-left"
  subnet_zone_bits              = var.subnet_zone_bits
  monitoring_network_ipv4_cidrs = var.monitoring_network_ipv4_cidrs
  use_local_compute_node        = var.use_local_compute_node

  worker_zones = concat(
    [for node in var.compute_nodes : node.zone if node.has_agent],
  [for node in var.compute_nodes : node.zone])
  worker_v4_cidr       = local.v4_left_inner_cidr
  worker_v6_cidr       = local.v6_left_inner_cidr
  routable_v4_cidr     = local.v4_right_inner_cidr
  routable_v6_cidr     = local.v6_right_inner_cidr
  router_instance_zone = local.ri_compute_node.zone
}

module "right" {
  source = "./k8s_network"

  folder_id                     = var.folder_id
  prefix                        = "${var.prefix}-right"
  subnet_zone_bits              = var.subnet_zone_bits
  monitoring_network_ipv4_cidrs = var.monitoring_network_ipv4_cidrs
  use_local_compute_node        = var.use_local_compute_node

  worker_zones         = [var.right_zone_id]
  worker_v4_cidr       = local.v4_right_inner_cidr
  worker_v6_cidr       = local.v6_right_inner_cidr
  routable_v4_cidr     = local.v4_left_inner_cidr
  routable_v6_cidr     = local.v6_left_inner_cidr
  router_instance_zone = local.ri_compute_node.zone
}

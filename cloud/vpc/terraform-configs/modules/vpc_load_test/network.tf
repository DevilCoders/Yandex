resource "ycp_vpc_network" "network" {
  // Well-known id of load_test folder in "yc.vpc.load_test" cloud. See https://bb.yandex-team.ru/projects/CLOUD/repos/bootstrap-templates/browse/terraform/*/vpc.tf
  folder_id   = var.folder_id

  name        = "load_test-network"
  description = "Network for load_test cluster"
}

resource "ycp_vpc_subnet" "subnets" {
  for_each       = toset(var.load_test_network_subnet_zones)

  folder_id      = var.folder_id
  network_id     = ycp_vpc_network.network.id

  name           = format("load_test-subnet-%s", each.key)
  zone_id        = each.key

  egress_nat_enable = true

  v4_cidr_blocks = [var.load_test_network_ipv4_cidrs[each.key]]
  v6_cidr_blocks = [cidrsubnet(
    cidrsubnet(
      var.vpc_load_test_network_ipv6_cidrs[each.key],
      32,
      var.vpc_overlay_net
    ),
    16,
    1)]

  extra_params {
    export_rts   = ["65533:666"]
    hbf_enabled  = var.load_test_network_hbf_enabled
    import_rts   = ["65533:776"]
    rpf_enabled  = false
  }
}

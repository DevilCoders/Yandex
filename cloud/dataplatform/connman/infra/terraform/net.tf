resource "yandex_vpc_network" "cm-net" {
  name = module.vars.net.net_name
}

resource "ycp_vpc_subnet" "cm-subnet" {
  for_each   = toset(local.zones)
  name       = "${module.vars.net.subnet_name_prefix}-${each.key}"
  network_id = yandex_vpc_network.cm-net.id
  v4_cidr_blocks = [
    module.vars.net.v4_cidr_blocks[each.key]
  ]
  v6_cidr_blocks = [
    module.vars.net.v6_cidr_blocks[each.key]
  ]
  zone_id           = each.key
  egress_nat_enable = true
  extra_params {
    export_rts = [
      "65533:666",
    ]
    feature_flags = [
      "hardened-public-ip",
      "blackhole",
    ]
    hbf_enabled = true
    import_rts = [
      "65533:776",
    ]
    rpf_enabled = false
  }
}
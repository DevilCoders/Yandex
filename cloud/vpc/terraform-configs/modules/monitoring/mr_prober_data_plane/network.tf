resource "ycp_vpc_network" "mr_prober_control" {
  folder_id   = var.folder_id

  name        = "control-network"
  description = "Control network for Mr.Prober"
}

resource "ycp_vpc_subnet" "mr_prober_control" {
  for_each       = toset(var.control_network_subnet_zones)

  folder_id      = var.folder_id
  network_id     = ycp_vpc_network.mr_prober_control.id

  name           = format("control-network-%s", each.key)
  zone_id        = each.key

  v4_cidr_blocks = [var.control_network_ipv4_cidrs[each.key]]
  v6_cidr_blocks = [var.control_network_ipv6_cidrs[each.key]]

  extra_params {
    export_rts   = ["65533:666"]
    hbf_enabled  = var.control_network_hbf_enabled
    import_rts   = ["65533:776"]
    rpf_enabled  = false
  }
}

resource "ycp_vpc_network" "default" {
  folder_id   = "yc.gore.service-folder"

  name        = "default"
  description = "Default network for GoRe Project"
}

resource "ycp_vpc_subnet" "default" {
  for_each       = toset(var.zones)

  folder_id      = "yc.gore.service-folder"
  network_id     = ycp_vpc_network.default.id

  name           = format("default-%s", each.key)
  zone_id        = each.key

  v4_cidr_blocks = [var.ipv4_cidrs[each.key]]
  v6_cidr_blocks = [cidrsubnet(var.ipv6_cidrs[each.key], 32, var.project_id)]

  egress_nat_enable = true

  extra_params {
    export_rts   = ["65533:666"]
    hbf_enabled  = var.hbf_enabled
    import_rts   = ["65533:776"]
    rpf_enabled  = false
  }

  lifecycle {
    ignore_changes = [
      extra_params[0].feature_flags
    ]
  }
}


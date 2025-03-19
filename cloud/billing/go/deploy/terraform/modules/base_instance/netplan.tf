locals {
  netplan_config = {
    network = {
      version = 2
      ethernets = merge(
        local.netplan_overlay,
        var.skip_underlay ? {} : local.netplan_underlay,
      )
    }
  }

  netplan_overlay = {
    eth0 = {
      dhcp4 = true
      dhcp6 = true
    },
  }

  netplan_underlay = {
    eth1 = {
      dhcp6 = true
      mtu   = 8950
      dhcp6-overrides = {
        route-metric = 101
      }
      routes = [
        for dst in local.underlay_routes :
        {
          to     = dst
          via    = "fe80::1"
          metric = 99
        }
      ]
    }
  }

  underlay_routes = [
    "2a0d:d6c0::/48", # CLOUD_INFRA_YONLY_VIPNETS
  ]
}

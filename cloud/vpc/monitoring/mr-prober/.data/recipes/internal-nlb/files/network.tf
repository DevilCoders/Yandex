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

  egress_nat_enable = true

  v4_cidr_blocks = [var.monitoring_network_ipv4_cidrs[each.key]]

  v6_cidr_blocks = [var.monitoring_network_ipv6_cidrs[each.key]]
  extra_params {
    export_rts   = ["65533:666"]
    hbf_enabled  = var.monitoring_network_hbf_enabled
    import_rts   = ["65533:776"]
    rpf_enabled  = false
  }
}

resource "ycp_vpc_security_group" "backends" {
  folder_id = var.folder_id

  network_id = ycp_vpc_network.network.id
  name = "${var.prefix}-backends-sg"
  description = "Security group for backends"

  rule_spec {
    direction = "INGRESS"
    protocol_name = "TCP"
    description = "Allow SSH"
    cidr_blocks {
      v6_cidr_blocks = ["2a02:6b8::/32"]
    }
    ports {
      from_port = 22
      to_port = 22
    }
  }

  rule_spec {
    direction = "INGRESS"
    protocol_name = "ICMP"
    description = "Allow pings"
    cidr_blocks {
      v4_cidr_blocks = ["0.0.0.0/0"]
    }
    ports {
      from_port = "0"
      to_port = "0"
    }
  }

  rule_spec {
    direction = "INGRESS"
    protocol_name = "IPV6_ICMP"
    description = "Allow pings"
    cidr_blocks {
      v6_cidr_blocks = ["::/0"]
    }
    ports {
      from_port = "0"
      to_port = "0"
    }
  }

  rule_spec {
    direction = "INGRESS"
    protocol_name = "TCP"
    description = "Allow 80/tcp by IPv4 and IPv6"
    cidr_blocks {
      v4_cidr_blocks = values(var.monitoring_network_ipv4_cidrs)
      v6_cidr_blocks = values(var.monitoring_network_ipv6_cidrs)
    }
    ports {
      from_port = 80
      to_port = 80
    }
  }

  rule_spec {
    direction = "INGRESS"
    protocol_name = "TCP"
    description = "Allow healthcheck by IPv4 and IPv6"
    cidr_blocks {
      v4_cidr_blocks = var.healthcheck_service_source_ipv4_networks
      v6_cidr_blocks = var.healthcheck_service_source_ipv6_networks
    }
    ports {
      from_port = 80
      to_port = 80
    }
  }

  rule_spec {
    direction = "EGRESS"
    protocol_name = "ANY"
    description = "Allow any egress traffic"
    cidr_blocks {
      v4_cidr_blocks = ["0.0.0.0/0"]
      v6_cidr_blocks = ["::/0"]
    }
    ports {
      from_port = 0
      to_port = 65535
    }
  }
}

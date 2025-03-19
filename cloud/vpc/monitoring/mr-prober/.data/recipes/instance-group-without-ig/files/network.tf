resource "ycp_vpc_network" "network" {
  count = var.create_monitoring_network ? 1 : 0

  folder_id   = var.folder_id

  name        = "${var.prefix}-network"
  description = "Network for cluster ${var.prefix}"

  depends_on = [
    ycp_resource_manager_folder.folder
  ]
}

resource "ycp_vpc_subnet" "subnets" {
  for_each       = toset(var.create_monitoring_network ? var.zones: [])

  folder_id      = var.folder_id
  network_id     = ycp_vpc_network.network[0].id

  name           = format("%s-network-%s", var.prefix, each.key)
  zone_id        = each.key

  egress_nat_enable = var.monitoring_network_enable_egress_nat

  v4_cidr_blocks = [var.monitoring_network_ipv4_cidrs[each.key]]

  v6_cidr_blocks = [var.monitoring_network_ipv6_cidrs[each.key]]

  extra_params {
    export_rts   = ["65533:666"]
    hbf_enabled  = var.monitoring_network_hbf_enabled
    import_rts   = ["65533:776"]
    rpf_enabled  = false
  }
}

resource "ycp_vpc_security_group" "agents" {
  count = var.create_monitoring_network ? 1 : 0

  folder_id = var.folder_id

  network_id = ycp_vpc_network.network[0].id
  name = "${var.prefix}-agents-sg"
  description = "Security group for instances in ${var.prefix}"

  rule_spec {
    direction = "INGRESS"
    protocol_name = "TCP"
    description = "Allow SSH"
    cidr_blocks {
      v6_cidr_blocks = ["2a02:6b8::/32", "2a11:f740::/48"]
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
  }

  rule_spec {
    direction = "INGRESS"
    protocol_name = "IPV6_ICMP"
    description = "Allow pings"
    cidr_blocks {
      v6_cidr_blocks = ["::/0"]
    }
  }

  rule_spec {
    direction = "INGRESS"
    protocol_name = "TCP"
    description = "Allow fetching metrics from unified-agent/solomon-agent"
    cidr_blocks {
      v6_cidr_blocks = ["2a02:6b8::/32", "2a11:f740::/48"]
    }
    ports {
      from_port = 8080
      to_port = 8080
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
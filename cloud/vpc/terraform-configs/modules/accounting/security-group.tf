resource "yandex_vpc_security_group" "accounting" {
  folder_id = var.folder_id
  network_id = ycp_vpc_network.network.id
  name = "accounting-sg"
  description = "Security group for accounting instances"

  ingress {
    protocol = "TCP"
    description = "Allow healthchecks"
    v4_cidr_blocks = ["198.18.235.0/24", "198.18.248.0/24"]
    v6_cidr_blocks = [var.hc_network_ipv6]
    port = 80
  }

  ingress {
    protocol = "TCP"
    description = "Allow SSH from Yandex"
    v6_cidr_blocks = ["2a02:6b8::/32"]
    port = 22
  }

  ingress {
    protocol = "TCP"
    description = "Allow 443 from accounting subnets"
    v6_cidr_blocks = [for k, v in var.accounting_network_ipv6_cidrs : v]
    port = 443
  }

  ingress {
    protocol = "TCP"
    description = "Allow web access from Yandex"
    v6_cidr_blocks = ["2a02:6b8::/32"]
    port = 80
  }

  ingress {
    protocol = "TCP"
    description = "Allow SSH from Yandex"
    v6_cidr_blocks = ["2a02:6b8::/32"]
    port = 22
  }

  ingress {
    protocol = "ANY"
    description = "Allow solomon"
    v6_cidr_blocks = ["2a02:6b8::/32"]
    from_port = 10000
    to_port = 10005
  }

  ingress {
    protocol = "ANY"
    description = "Allow inter-group communication"
    predefined_target = "self_security_group"
    from_port = 0
    to_port = 65535
  }

  egress {
    protocol = "ANY"
    description = "Allow any egress traffic"
    v4_cidr_blocks = ["0.0.0.0/0"]
    v6_cidr_blocks = ["::/0"]
    from_port = 0
    to_port = 65535
  }
}
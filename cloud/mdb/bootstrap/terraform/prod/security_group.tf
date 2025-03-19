locals {
  v6-yandex-and-cloud = ["2a02:6b8::/32", "2a0d:d6c0::/29"]
}

resource "yandex_vpc_security_group" "control-plane-sg" {
  name        = "sg-cp"
  description = "Control plane security group"
  network_id  = ycp_vpc_network.dbaasexternalnets.id

  # Ingress rules
  ingress {
    protocol       = "TCP"
    description    = "Allow incoming traffic to SSH"
    port           = 22
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  ingress {
    protocol       = "TCP"
    description    = "Ingress HTTP/GRPC"
    port           = 443
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  ingress {
    protocol       = "TCP"
    description    = "Ingress HTTP 80"
    port           = 80
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  ingress {
    protocol       = "UDP"
    description    = "Ingress DNS UDP"
    port           = 53
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  ingress {
    protocol          = "ANY"
    description       = "Self INGRESS"
    from_port         = 0
    to_port           = 65535
    predefined_target = "self_security_group"
  }

  ingress {
    protocol       = "IPV6_ICMP"
    description    = "Ingress ICMP"
    v6_cidr_blocks = local.v6-yandex-and-cloud
    from_port      = 0
    to_port        = 32767
  }

  # Temp ingress rules (will be removed after move all services to this SG)
  ingress {
    description    = "PostgreSQL pooler"
    protocol       = "TCP"
    port           = 6432
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  ingress {
    description    = "PostgreSQL"
    protocol       = "TCP"
    port           = 5432
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  ingress {
    description    = "Zookeeper Client"
    protocol       = "TCP"
    port           = 2181
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  ingress {
    description    = "Zookeeper Followers"
    protocol       = "TCP"
    port           = 2888
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  ingress {
    description    = "Zookeeper Election"
    protocol       = "TCP"
    port           = 3888
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  ingress {
    description    = "Redis Sentinel"
    protocol       = "TCP"
    port           = 26379
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  ingress {
    description    = "Redis"
    protocol       = "TCP"
    port           = 6379
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  ingress {
    description    = "YASM 3"
    protocol       = "TCP"
    port           = 11003
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  ingress {
    description    = "YASM 5"
    protocol       = "TCP"
    port           = 11005
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  ingress {
    description    = "Salt"
    protocol       = "TCP"
    from_port      = 4505
    to_port        = 4506
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  # Egress rules
  # Allow ICMP IPv6, TCP, UDP on non-ephemeral ports.
  egress {
    protocol       = "IPV6_ICMP"
    description    = "Egress ICMP"
    v6_cidr_blocks = local.v6-yandex-and-cloud
    from_port      = 0
    to_port        = 32767
  }

  egress {
    protocol       = "TCP"
    description    = "Egress TCP"
    v6_cidr_blocks = local.v6-yandex-and-cloud
    from_port      = 0
    to_port        = 32767
  }

  egress {
    protocol       = "UDP"
    description    = "Egress UDP"
    v6_cidr_blocks = local.v6-yandex-and-cloud
    from_port      = 0
    to_port        = 32767
  }

  egress {
    protocol          = "ANY"
    description       = "Self EGRESS"
    from_port         = 0
    to_port           = 65535
    predefined_target = "self_security_group"
  }
}

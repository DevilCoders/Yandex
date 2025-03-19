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

resource "yandex_vpc_security_group" "dualstack-nets" {
  name        = "dualstack-nets"
  description = "control plane dual stack network"
  network_id  = ycp_vpc_network.mdb_controlplane_dualstacknets-nets.id

  ingress {
    protocol       = "TCP"
    description    = "go int api uses ClickHouse"
    v6_cidr_blocks = ["2a02:6b8::/32", "2a0d:d6c0::/29"]
    port           = 9440
  }
  ingress {
    protocol       = "TCP"
    description    = "balancer health check https://wiki.yandex-team.ru/cloud/devel/loadbalancing/ipv6/#securitygroupsihealthchecks"
    v4_cidr_blocks = ["198.18.235.0/24", "198.18.248.0/24"]
    v6_cidr_blocks = ["2a0d:d6c0:2:ba::/80"]
    port           = 80
  }
  ingress {
    protocol       = "TCP"
    description    = "allow SSH access"
    v4_cidr_blocks = ["10.0.0.0/24"] # local network
    v6_cidr_blocks = local.v6-yandex-and-cloud
    port           = 22
  }
  ingress {
    protocol       = "TCP"
    description    = "allow MDB SSH access"
    v4_cidr_blocks = ["10.0.0.0/24"] # local network
    v6_cidr_blocks = local.v6-yandex-and-cloud
    port           = 22
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
    protocol          = "TCP"
    description       = "allow local-network traffic so frontend->backend works"
    predefined_target = "self_security_group"
    port              = 80 // use self
  }
  egress {
    protocol       = "ANY"
    description    = "to download images from cr.yandex" // cr.yandex and storage.yandexcloud.net :443 is enough
    v4_cidr_blocks = ["0.0.0.0/0"]
    v6_cidr_blocks = ["::/0"]
    from_port      = 0
    to_port        = 65535
  }
}

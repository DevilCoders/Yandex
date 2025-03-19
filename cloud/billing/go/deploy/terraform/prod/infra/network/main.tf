locals {
  billing_service_folder = var.yc_folder
  network_name           = "billing-nets"
}

locals {
  v6-yandex           = ["2a02:6b8::/32"]
  v6-cloud            = ["2a0d:d6c0::/29"]
  v6-yandex-and-cloud = ["2a02:6b8::/32", "2a0d:d6c0::/29"]
  v4-lb-healthchecks  = ["198.18.235.0/24", "198.18.248.0/24"]
  v6-lb-healthchecks  = ["2a0d:d6c0:2:ba::/64"]
  v4-any              = ["0.0.0.0/0"]
  v6-any              = ["::/0"]

  v4-billing-ch = ["172.16.0.0/16"]
}


data "yandex_vpc_network" "net" {
  name = local.network_name
}

resource "yandex_vpc_security_group" "piper" {
  name        = "piper-security-group"
  description = "security group for piper hosts"
  network_id  = data.yandex_vpc_network.net.id

  ingress {
    protocol       = "TCP"
    description    = "Allow SSH from Yandex and Yandex.Cloud"
    v6_cidr_blocks = local.v6-yandex-and-cloud
    port           = 22
  }

  # No API - no rules
  #   ingress {
  #     protocol       = "TCP"
  #     description    = "Allow healthchecks from Yandex.Cloud services (lb, ig, alb....)"
  #     v4_cidr_blocks = local.v4-lb-healthchecks
  #     v6_cidr_blocks = local.v6-lb-healthchecks
  #     port           = 30080
  #   }

  ingress {
    protocol       = "IPV6_ICMP"
    description    = "Allow ICMPv6 from Yandex and Yandex.Cloud"
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  ingress {
    protocol          = "ANY"
    description       = "Allow incoming traffic from members of the same security group"
    from_port         = 0
    to_port           = 65535
    predefined_target = "self_security_group"
  }

  egress {
    protocol       = "IPV6_ICMP"
    description    = "Allow ICMPv6 to Yandex and Yandex.Cloud"
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  egress {
    protocol       = "TCP"
    description    = "Allow TCP for non-ephemeral ports at Yandex and Yandex.Cloud"
    v6_cidr_blocks = local.v6-yandex-and-cloud # We want to access lb at yande-team
    from_port      = 0
    to_port        = 32767
  }

  egress {
    protocol       = "ANY"
    description    = "Allow DNS at Yandex"
    v6_cidr_blocks = local.v6-yandex # We want to access lb at yande-team
    port           = 53
  }

  egress {
    protocol          = "ANY"
    description       = "Allow outgoing traffic to members of the same security group"
    from_port         = 0
    to_port           = 65535
    predefined_target = "self_security_group"
  }

  egress {
    protocol       = "TCP"
    description    = "CH access"
    from_port      = 8000
    to_port        = 9999
    v4_cidr_blocks = local.v4-billing-ch
  }
}

output "piper-security-group" {
  value = yandex_vpc_security_group.piper
}

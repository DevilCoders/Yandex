terraform {
  required_providers {
    yandex = {
      source = "registry.terraform.io/yandex-cloud/yandex"
    }
  }
  required_version = ">= 0.13"
}

provider "yandex" {
  endpoint                 = "${var.yc_endpoint}"
  folder_id                = "${var.yc_folder}"
  zone                     = "${var.yc_zone}"
  service_account_key_file = "${var.yc_service_account_key_file}"
}

locals {
  v6-yandex           = ["2a02:6b8::/32"]
  v6-cloud            = ["2a0d:d6c0::/29"]
  v6-yandex-and-cloud = ["2a02:6b8::/32", "2a0d:d6c0::/29"]
  v4-any              = ["0.0.0.0/0"]
  v6-any              = ["::/0"]
  v4-l4lb-healthcheck = ["198.18.235.0/24", "198.18.248.0/24"]
}

resource yandex_vpc_security_group common-security-group {
  name        = "common-security-group"
  description = "Common security group"
  network_id  = "${var.network_id}"

  //############################################################
  //ingress rules
  //############################################################

  // ssh
  ingress {
    protocol       = "TCP"
    description    = "Allow SSH from Yandex and Yandex.Cloud"
    port           = 22
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  // ipv6 icmp
  ingress {
    protocol       = "IPV6_ICMP"
    description    = "Allow ICMPv6 from Yandex and Yandex.Cloud"
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  // self
  ingress {
    protocol          = "ANY"
    description       = "Allow incoming traffic from members of the same security group"
    from_port         = 0
    to_port           = 65535
    predefined_target = "self_security_group"
  }

  // public api
  ingress {
    protocol       = "TCP"
    description    = "Allow Public API HTTPS/GRPC"
    port           = 443
    v4_cidr_blocks = local.v4-any
    v6_cidr_blocks = local.v6-any
  }

  // private api
  ingress {
    protocol       = "TCP"
    description    = "Allow Private API HTTPS/GRPC"
    port           = 8443
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  // l4 balancer healthchek
  ingress {
    protocol       = "TCP"
    description    = "Allow L4 Balancer to check healthcheck"
    port           = 9982
    v4_cidr_blocks = local.v4-l4lb-healthcheck
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  // l4 balancer healthchek
  ingress {
    protocol       = "TCP"
    description    = "Allow L4 Balancer to check healthcheck"
    port           = 444
    v4_cidr_blocks = local.v4-l4lb-healthcheck
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  // solomon pull
  ingress {
    protocol       = "TCP"
    description    = "Allow Solomon to pull metrics"
    port           = 8080
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  // solomon diagnostic pull
  ingress {
    protocol       = "TCP"
    description    = "Allow Solomon to pull diagnostic metrics"
    port           = 8081
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  //############################################################
  //egress rules
  //############################################################

  // ipv6 icmp
  egress {
    protocol       = "IPV6_ICMP"
    description    = "Allow ICMPv6 to Yandex and Yandex.Cloud"
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }

  // tcp for internet
  egress {
    protocol       = "TCP"
    description    = "Allow TCP for Internet"
    from_port      = 0
    to_port        = 65535
    v4_cidr_blocks = local.v4-any
    v6_cidr_blocks = local.v6-any
  }

  // udp for internet to check dns records
  egress {
    protocol       = "UDP"
    description    = "Allow UDP for Internet to check DNS records"
    port           = 53
    v4_cidr_blocks = local.v4-any
    v6_cidr_blocks = local.v6-any
  }

  // self
  egress {
    protocol          = "ANY"
    description       = "Allow outgoing traffic to members of the same security group"
    from_port         = 0
    to_port           = 65535
    predefined_target = "self_security_group"
  }

  // udp for yandex and coud
  egress {
    protocol       = "UDP"
    description    = "Allow UDP to yandex and cloud"
    from_port      = 0
    to_port        = 65535
    v6_cidr_blocks = local.v6-yandex-and-cloud
  }
}

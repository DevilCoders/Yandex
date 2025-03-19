provider "yandex" {
  endpoint                 = "${var.yc_endpoint}"
  folder_id                = "${var.yc_folder}"
  zone                     = "${var.yc_zone}"
  token                    = "${var.yc_token}"
}

locals {
  v6-yandex           = ["2a02:6b8::/32"]
  v6-cloud            = ["2a0d:d6c0::/29"]
  v6-yandex-and-cloud = ["2a02:6b8::/32", "2a0d:d6c0::/29"]
  v4-any              = ["0.0.0.0/0"]
  v6-any              = ["::/0"]
  v4-ylb-health-check = ["198.18.235.0/24", "198.18.248.0/24"]
  v6-ylb-health-check = ["2a0d:d6c0:2:ba::/80"]
}

resource yandex_vpc_security_group default-security-group {
  name        = "default-sg-${var.network_id}"
  description = "Default security group for ${var.network_id}"
  network_id  = "${var.network_id}"

  //############################################################
  //ingress rules
  //############################################################

  // ssh
  ingress {
    protocol          = "TCP"
    description       = "Allow SSH from Yandex and Yandex.Cloud"
    port              = 22
    v6_cidr_blocks    = local.v6-yandex-and-cloud
  }

  // ipv6 icmp
  ingress {
    protocol          = "IPV6_ICMP"
    description       = "Allow ICMPv6 from Yandex and Yandex.Cloud"
    v6_cidr_blocks    = local.v6-yandex-and-cloud
  }

  // self
  ingress {
    protocol          = "ANY"
    description       = "Allow incoming traffic from members of the same security group"
    from_port         = 0
    to_port           = 65535
    predefined_target = "self_security_group"
  }

  // api
  ingress {
    protocol          = "TCP"
    description       = "Allow API HTTPS/GRPC"
    port              = 443
    v4_cidr_blocks    = local.v4-any
    v6_cidr_blocks    = local.v6-any
  }

  // solomon pull
  ingress {
    protocol          = "TCP"
    description       = "Allow Solomon to pull metrics"
    port              = 8080
    v6_cidr_blocks    = local.v6-yandex-and-cloud
  }

  // solomon diagnostics pull
  ingress {
    protocol          = "TCP"
    description       = "Allow Solomon to pull diagnostic metrics"
    port              = 8081
    v6_cidr_blocks    = local.v6-yandex-and-cloud
  }

  // ydb load-balancer health-check https://cloud.yandex.ru/docs/network-load-balancer/concepts/health-check#target-statuses
  ingress {
    protocol          = "TCP"
    description       = "Allow TCP to LB YDB health-checks https://cloud.yandex.ru/docs/network-load-balancer/concepts/health-check#target-statuses"
    port              = 2135
    v4_cidr_blocks    = local.v4-ylb-health-check
    v6_cidr_blocks    = local.v6-ylb-health-check
  }

  // alb load-balancer nodes health-check https://wiki.yandex-team.ru/cloud/devel/platform-team/l7/usage/#0prerekvizity
  ingress {
    protocol          = "TCP"
    description       = "Allow TCP to ALB nodes health-checks https://wiki.yandex-team.ru/cloud/devel/platform-team/l7/usage/#0prerekvizity"
    port              = 30080
    v4_cidr_blocks    = local.v4-ylb-health-check
    v6_cidr_blocks    = local.v6-ylb-health-check
  }

  //############################################################
  //egress rules
  //############################################################

  // ipv6 icmp
  egress {
    protocol          = "IPV6_ICMP"
    description       = "Allow ICMPv6 to Yandex and Yandex.Cloud"
    v6_cidr_blocks    = local.v6-yandex-and-cloud
  }

  // tcp for yandex and cloud
  egress {
    protocol          = "TCP"
    description       = "Allow TCP to yandex and cloud"
    from_port         = 0
    to_port           = 32767
    v6_cidr_blocks    = local.v6-yandex-and-cloud
  }

  // udp for yandex and cloud
  egress {
    protocol          = "UDP"
    description       = "Allow UDP to yandex and cloud"
    from_port         = 0
    to_port           = 32767
    v6_cidr_blocks    = local.v6-yandex-and-cloud
  }

  // self
  egress {
    protocol          = "ANY"
    description       = "Allow outgoing traffic to members of the same security group"
    from_port         = 0
    to_port           = 65535
    predefined_target = "self_security_group"
  }
}

locals {
  yandexnets = {
    ipv4 = [
      "5.45.192.0/18",
      "5.255.192.0/18",
      "37.9.64.0/18",
      "37.140.128.0/18",
      "45.87.132.0/22",
      "77.88.0.0/18",
      "87.250.224.0/19",
      "90.156.177.0/24",
      "93.158.128.0/19",
      "93.158.160.0/20",
      "95.108.128.0/17",
      "100.43.64.0/19",
      "139.45.249.96/29", // https://st.yandex-team.ru/NOCREQUESTS-25161
      "141.8.128.0/18",
      "178.154.128.0/19",
      "178.154.160.0/19",
      "199.21.96.0/22",
      "199.36.240.0/22",
      "213.180.192.0/19",
    ]
    ipv6 = [
      "2620:10f:d000::/44",
      "2a02:6b8::/32",
    ]
  }
}

resource "yandex_vpc_security_group" "cloudbeaver-devhost-sg" {
  name       = "cloudbeaver-devhost"
  network_id = data.ycp_vpc_network.cloudbeaver.id
  ingress {
    protocol       = "ANY"
    description    = "ANY in"
    v4_cidr_blocks = ["0.0.0.0/0"]
    v6_cidr_blocks = ["::/0"]
    from_port      = 0
    to_port        = 65535
  }
  egress {
    protocol       = "ANY"
    description    = "ANY out"
    v4_cidr_blocks = ["0.0.0.0/0"]
    v6_cidr_blocks = ["::/0"]
    from_port      = 0
    to_port        = 65535
  }
}

resource "yandex_vpc_security_group" "cloudbeaver-sg" {
  name        = "cloudbeaver"
  description = "CloduBeaver default security group"
  network_id  = data.ycp_vpc_network.cloudbeaver.id

  ingress {
    protocol       = "TCP"
    description    = "Allow incoming traffic to 22 (SSH) from yandexnets"
    port           = 22
    v4_cidr_blocks = local.yandexnets.ipv4
    v6_cidr_blocks = local.yandexnets.ipv6
  }

  ingress {
    protocol       = "TCP"
    description    = "Allow incominng traffic to 443 (HTTPS) from yandexnets"
    port           = 443
    v4_cidr_blocks = local.yandexnets.ipv4
    v6_cidr_blocks = local.yandexnets.ipv6
  }

  ingress {
    protocol       = "TCP"
    description    = "Allow incoming traffic to 80 (HTTP) from yandexnets"
    port           = 80
    v4_cidr_blocks = local.yandexnets.ipv4
    v6_cidr_blocks = local.yandexnets.ipv6
  }

  ingress {
    protocol       = "ICMP"
    description    = "Allow incoming ICMP traffic from yandexnets"
    v4_cidr_blocks = local.yandexnets.ipv4
    v6_cidr_blocks = local.yandexnets.ipv6
    from_port      = 0
    to_port        = 32767
  }

  ingress {
    protocol          = "ANY"
    description       = "Allow any incoming traffic to sg members"
    from_port         = 0
    to_port           = 65535
    predefined_target = "self_security_group"
  }
  egress {
    protocol          = "ANY"
    description       = "Allow any outgoing traffic to sg members"
    from_port         = 0
    to_port           = 65535
    predefined_target = "self_security_group"
  }
}

resource "yandex_vpc_network" "xrdp" {
  name = local.name
}

resource "yandex_vpc_subnet" "xrdp" {
  network_id     = yandex_vpc_network.xrdp.id
  name           = local.name
  v4_cidr_blocks = [var.subnet_v4_cidr_block]
  zone           = var.zone
}

resource "yandex_vpc_security_group" "xrdp" {
  name       = local.name
  network_id = yandex_vpc_network.xrdp.id
}

resource "yandex_vpc_security_group_rule" "allow_internet" {
  security_group_binding = yandex_vpc_security_group.xrdp.id
  direction              = "egress"
  description            = "RDP"
  protocol               = "ANY"
  v4_cidr_blocks         = ["0.0.0.0/0"]
}

resource "yandex_vpc_security_group_rule" "allow_rdp" {
  security_group_binding = yandex_vpc_security_group.xrdp.id
  direction              = "ingress"
  description            = "RDP"
  protocol               = "ANY"
  v4_cidr_blocks         = ["0.0.0.0/0"]
  port                   = 3389
}

resource "yandex_vpc_security_group_rule" "allow_ssh" {
  security_group_binding = yandex_vpc_security_group.xrdp.id
  direction              = "ingress"
  description            = "to rds"
  protocol               = "ANY"
  v4_cidr_blocks         = ["0.0.0.0/0"]
  port                   = 22
}

resource "yandex_vpc_security_group_rule" "allow_icmp" {
  security_group_binding = yandex_vpc_security_group.xrdp.id
  direction              = "ingress"
  description            = "ICMP"
  protocol               = "ICMP"
  v4_cidr_blocks         = ["0.0.0.0/0"]
}

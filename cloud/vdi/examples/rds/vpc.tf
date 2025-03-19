resource "yandex_vpc_network" "default" {
  name = local.network_name
}

# Security Groups

locals {
  network_name = join("", ["rds-", random_string.launch_code.result])
  subnet_name  = join("", ["rds-", random_string.launch_code.result])

  ad_security_group_name  = join("", ["rds-", random_string.launch_code.result, "-ad"])
  rds_security_group_name = join("", ["rds-", random_string.launch_code.result, "-rds"])
}

resource "yandex_vpc_security_group" "ad" {
  name       = local.ad_security_group_name
  network_id = yandex_vpc_network.default.id
}

resource "yandex_vpc_security_group_rule" "ad_allow_self" {
  security_group_binding = yandex_vpc_security_group.ad.id
  direction              = "ingress"
  description            = "SELF"
  protocol               = "ANY"
  predefined_target      = "self_security_group"
}

resource "yandex_vpc_security_group_rule" "ad_allow_rdp" {
  security_group_binding = yandex_vpc_security_group.ad.id
  direction              = "ingress"
  description            = "RDP"
  protocol               = "ANY"
  v4_cidr_blocks         = ["0.0.0.0/0"]
  port                   = 3389
}

resource "yandex_vpc_security_group_rule" "ad_allow_icmp" {
  security_group_binding = yandex_vpc_security_group.ad.id
  direction              = "ingress"
  description            = "ICMP"
  protocol               = "ICMP"
  v4_cidr_blocks         = ["0.0.0.0/0"]
}

resource "yandex_vpc_security_group_rule" "ad_allow_rds" {
  security_group_binding = yandex_vpc_security_group.ad.id
  direction              = "egress"
  description            = "to rds"
  protocol               = "ANY"
  security_group_id      = yandex_vpc_security_group.rds.id
}

resource "yandex_vpc_security_group_rule" "ad_allow_from_rds" {
  security_group_binding = yandex_vpc_security_group.ad.id
  direction              = "ingress"
  description            = "to rds"
  protocol               = "ANY"
  security_group_id      = yandex_vpc_security_group.rds.id
}

resource "yandex_vpc_security_group" "rds" {
  name       = local.rds_security_group_name
  network_id = yandex_vpc_network.default.id
}

resource "yandex_vpc_security_group_rule" "rds_allow_self" {
  security_group_binding = yandex_vpc_security_group.rds.id
  direction              = "ingress"
  description            = "SELF"
  protocol               = "ANY"
  predefined_target      = "self_security_group"
}

resource "yandex_vpc_security_group_rule" "rds_allow_rdp" {
  security_group_binding = yandex_vpc_security_group.rds.id
  direction              = "ingress"
  description            = "RDP"
  protocol               = "ANY"
  v4_cidr_blocks         = ["0.0.0.0/0"]
  port                   = 3389
}

resource "yandex_vpc_security_group_rule" "rds_allow_icmp" {
  security_group_binding = yandex_vpc_security_group.rds.id
  direction              = "ingress"
  description            = "ICMP"
  protocol               = "ICMP"
  v4_cidr_blocks         = ["0.0.0.0/0"]
}

resource "yandex_vpc_security_group_rule" "rds_allow_ad" {
  security_group_binding = yandex_vpc_security_group.rds.id
  direction              = "egress"
  description            = "to ad"
  protocol               = "ANY"
  security_group_id      = yandex_vpc_security_group.ad.id
}

resource "yandex_vpc_security_group_rule" "rds_allow_from_ad" {
  security_group_binding = yandex_vpc_security_group.rds.id
  direction              = "ingress"
  description            = "from ad"
  protocol               = "ANY"
  security_group_id      = yandex_vpc_security_group.ad.id
}

resource "yandex_vpc_security_group_rule" "rds_allow_internet" {
  security_group_binding = yandex_vpc_security_group.rds.id
  direction              = "egress"
  description            = "RDP"
  protocol               = "ANY"
  v4_cidr_blocks         = ["0.0.0.0/0"]
}

resource "yandex_vpc_subnet" "default" {
  network_id     = yandex_vpc_network.default.id
  name           = local.subnet_name
  v4_cidr_blocks = [var.subnet_v4_cidr_block]
  zone           = var.zone

  dhcp_options {
    domain_name = local.domain_name
    domain_name_servers = [local.ad_ip_address]
  }
}

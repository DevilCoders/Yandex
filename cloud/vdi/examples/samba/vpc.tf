resource "yandex_vpc_network" "default" {
  name = local.network_name
}

# Security Groups

locals {
  network_name = join("", ["rds-", random_string.launch_code.result])
  samba_subnet_name  = join("", ["samba-", random_string.launch_code.result])
  rds_subnet_name  = join("", ["rds-", random_string.launch_code.result])

  samba_security_group_name  = join("", ["rds-", random_string.launch_code.result, "-samba"])
  rds_security_group_name = join("", ["rds-", random_string.launch_code.result, "-rds"])
}

resource "yandex_vpc_security_group" "samba" {
  name       = local.samba_security_group_name
  network_id = yandex_vpc_network.default.id
}

resource "yandex_vpc_security_group_rule" "samba_allow_self" {
  security_group_binding = yandex_vpc_security_group.samba.id
  direction              = "ingress"
  description            = "SELF"
  protocol               = "ANY"
  predefined_target      = "self_security_group"
}

resource "yandex_vpc_security_group_rule" "samba_allow_ssh" {
  security_group_binding = yandex_vpc_security_group.samba.id
  direction              = "ingress"
  description            = "SSH"
  protocol               = "ANY"
  v4_cidr_blocks         = ["0.0.0.0/0"]
  port                   = 22
}

resource "yandex_vpc_security_group_rule" "samba_allow_icmp" {
  security_group_binding = yandex_vpc_security_group.samba.id
  direction              = "ingress"
  description            = "ICMP"
  protocol               = "ICMP"
  v4_cidr_blocks         = ["0.0.0.0/0"]
}

resource "yandex_vpc_security_group_rule" "samba_allow_internet" {
  security_group_binding = yandex_vpc_security_group.samba.id
  direction              = "egress"
  description            = "ANY"
  protocol               = "ANY"
  v4_cidr_blocks         = ["0.0.0.0/0"]
}

resource "yandex_vpc_security_group_rule" "samba_allow_rds" {
  security_group_binding = yandex_vpc_security_group.samba.id
  direction              = "egress"
  description            = "to rds"
  protocol               = "ANY"
  security_group_id      = yandex_vpc_security_group.rds.id
}

resource "yandex_vpc_security_group_rule" "samba_allow_from_rds" {
  security_group_binding = yandex_vpc_security_group.samba.id
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

resource "yandex_vpc_security_group_rule" "rds_allow_samba" {
  security_group_binding = yandex_vpc_security_group.rds.id
  direction              = "egress"
  description            = "to samba"
  protocol               = "ANY"
  security_group_id      = yandex_vpc_security_group.samba.id
}

resource "yandex_vpc_security_group_rule" "rds_allow_from_samba" {
  security_group_binding = yandex_vpc_security_group.rds.id
  direction              = "ingress"
  description            = "from samba"
  protocol               = "ANY"
  security_group_id      = yandex_vpc_security_group.samba.id
}

resource "yandex_vpc_security_group_rule" "rds_allow_internet" {
  security_group_binding = yandex_vpc_security_group.rds.id
  direction              = "egress"
  description            = "RDP"
  protocol               = "ANY"
  v4_cidr_blocks         = ["0.0.0.0/0"]
}

resource "yandex_vpc_subnet" "samba" {
  network_id     = yandex_vpc_network.default.id
  name           = local.samba_subnet_name
  v4_cidr_blocks = [var.samba_v4_cidr_block]
  zone           = var.zone
}

resource "yandex_vpc_subnet" "rds" {
  network_id     = yandex_vpc_network.default.id
  name           = local.rds_subnet_name
  v4_cidr_blocks = [var.rds_v4_cidr_block]
  zone           = var.zone

  dhcp_options {
    domain_name = local.realm_name
    domain_name_servers = [local.samba_ip_address]
  }
}

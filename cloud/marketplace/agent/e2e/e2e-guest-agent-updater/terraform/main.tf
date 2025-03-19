terraform {
  required_providers {
    yandex = {
      source = "yandex-cloud/yandex"
    }
  }
}

provider "yandex" {
  cloud_id  = var.cloud_id
  folder_id = var.folder_id
  zone      = var.zone
  token     = var.token
}

locals {
  uuid = element(split("-", uuid()), 5)

  instance_name       = join("", [var.name_prefix, "-pwd-reset-", local.uuid])
  network_name        = join("", [var.name_prefix, "-pwd-reset-", local.uuid])
  subnet_name         = join("", [var.name_prefix, "-pwd-reset-", local.uuid])
  security_group_name = join("", [var.name_prefix, "-pwd-reset-", local.uuid])
}

resource "yandex_vpc_network" "default" {
  name = local.network_name
}

resource "yandex_vpc_subnet" "default" {
  network_id     = yandex_vpc_network.default.id
  name           = local.subnet_name
  v4_cidr_blocks = var.subnet_v4_cidr_blocks
  zone           = var.zone
}

resource "yandex_vpc_security_group" "default" {
  name       = local.security_group_name
  network_id = yandex_vpc_network.default.id

  ingress {
    description       = "SELF"
    protocol          = "ANY"
    predefined_target = "self_security_group"
  }

  ingress {
    description    = "ICMP"
    protocol       = "ICMP"
    v4_cidr_blocks = ["0.0.0.0/0"]
  }

  ingress {
    description    = "RDP"
    protocol       = "ANY"
    v4_cidr_blocks = ["0.0.0.0/0"]
    port           = 3389
  }

  ingress {
    description    = "WINRM-HTTPS"
    protocol       = "ANY"
    v4_cidr_blocks = ["0.0.0.0/0"]
    port           = 5986
  }

  egress {
    description    = "ANY"
    protocol       = "ANY"
    v4_cidr_blocks = ["0.0.0.0/0"]
  }
}

data "yandex_compute_image" "default" {
  family = var.image_family
}

data "template_file" "default" {
  template = file("${path.module}/init.ps1")
  vars     = {
    admin_pass = var.admin_pass
  }
}

resource "yandex_compute_instance" "default" {
  name        = local.instance_name
  platform_id = "standard-v2"
  zone        = var.zone

  resources {
    cores  = var.cores
    memory = var.memory
  }

  boot_disk {
    initialize_params {
      image_id = data.yandex_compute_image.default.id
      size     = var.disk_size
      type     = var.disk_type
    }
  }

  network_interface {
    subnet_id          = yandex_vpc_subnet.default.id
    nat                = var.nat
    security_group_ids = [yandex_vpc_security_group.default.id]
  }

  metadata = {
    user-data = data.template_file.default.rendered
  }

  timeouts {
    create = var.timeout_create
    delete = var.timeout_delete
  }
}

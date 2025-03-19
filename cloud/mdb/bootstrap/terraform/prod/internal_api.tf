resource "ycp_compute_instance" "api-admin-dbaas01k" {
  name                      = "api-admin-dbaas01k"
  zone_id                   = local.zones.zone_a.id
  platform_id               = "standard-v1"
  fqdn                      = "api-admin-dbaas01k.yandexcloud.net"
  allow_stopping_for_update = true

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 20
    cores         = 2
    memory        = 4
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 30
      image_id = "fd8dmei853vqamqse50s"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_a.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "api-dbaas01f" {
  name        = "api-dbaas01f"
  zone_id     = local.zones.zone_c.id
  platform_id = "standard-v1"
  fqdn        = "api-dbaas01f.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 8
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 30
      image_id = "fd8dmei853vqamqse50s"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_c.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "api-dbaas01h" {
  name        = "api-dbaas01h"
  zone_id     = local.zones.zone_b.id
  platform_id = "standard-v1"
  fqdn        = "api-dbaas01h.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 8
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 30
      image_id = "fd8dmei853vqamqse50s"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_b.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "api-dbaas01k" {
  name        = "api-dbaas01k"
  zone_id     = local.zones.zone_a.id
  platform_id = "standard-v1"
  fqdn        = "api-dbaas01k.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 8
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 30
      image_id = "fd8dmei853vqamqse50s"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_a.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "api-dbaas02f" {
  name        = "api-dbaas02f"
  zone_id     = local.zones.zone_c.id
  platform_id = "standard-v1"
  fqdn        = "api-dbaas02f.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 8
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 30
      image_id = "fd8dmei853vqamqse50s"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_c.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "api-dbaas02h" {
  name        = "api-dbaas02h"
  zone_id     = local.zones.zone_b.id
  platform_id = "standard-v1"
  fqdn        = "api-dbaas02h.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 8
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 30
      image_id = "fd8dmei853vqamqse50s"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_b.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "api-dbaas02k" {
  name        = "api-dbaas02k"
  zone_id     = local.zones.zone_a.id
  platform_id = "standard-v1"
  fqdn        = "api-dbaas02k.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 8
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 30
      image_id = "fd8dmei853vqamqse50s"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_a.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_load_balancer_target_group" "api-dbaas" {
  name      = "api-dbaas"
  region_id = var.region_id
  target {
    address   = ycp_compute_instance.api-dbaas01f.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.api-dbaas01f.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.api-dbaas01k.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.api-dbaas01k.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.api-dbaas01h.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.api-dbaas01h.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.api-dbaas02f.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.api-dbaas02f.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.api-dbaas02k.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.api-dbaas02k.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.api-dbaas02h.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.api-dbaas02h.network_interface.0.subnet_id
  }
}

resource "ycp_load_balancer_network_load_balancer" "api-dbaas" {
  name      = "api-dbaas-lb"
  region_id = var.region_id
  type      = "EXTERNAL"
  attached_target_group {
    target_group_id = ycp_load_balancer_target_group.api-dbaas.id
    health_check {
      healthy_threshold   = 2
      interval            = "5s"
      name                = "ping"
      timeout             = "2s"
      unhealthy_threshold = 2
      http_options {
        path = "/ping"
        port = 80
      }
    }
  }
  listener_spec {
    port        = 443
    target_port = 443
    name        = "api-dbaas"
    protocol    = "TCP"
    external_address_spec {
      yandex_only = true
      ip_version  = "IPV6"
      address     = "2a0d:d6c0:0:1b::308"
    }
  }
}

resource "ycp_iam_service_account" "internal-api" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "internal-api"
  service_account_id = "yc.mdb.internal-api"
}

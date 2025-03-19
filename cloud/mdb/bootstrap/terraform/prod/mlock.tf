locals {
  mlock_boot_disk_spec = {
    size     = 30
    image_id = "fd82uv5tph35d0lcgc7r"
    type_id  = "network-hdd"
  }
}

resource "yandex_iam_service_account" "mlockmonitor" {
  name = "mlockmonitor"
}

resource "yandex_iam_service_account" "mlockworker" {
  name = "mlockworker"
}

resource "ycp_compute_instance" "mlockdb01f" {
  name        = "mlockdb01f"
  zone_id     = local.zones.zone_c.id
  platform_id = "standard-v2"
  fqdn        = "mlockdb01f.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 50
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = local.mlock_boot_disk_spec.size
      image_id = local.mlock_boot_disk_spec.image_id
      type_id  = local.mlock_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-c.id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mlockdb01h" {
  name        = "mlockdb01h"
  zone_id     = local.zones.zone_b.id
  platform_id = "standard-v2"
  fqdn        = "mlockdb01h.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 50
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = local.mlock_boot_disk_spec.size
      image_id = local.mlock_boot_disk_spec.image_id
      type_id  = local.mlock_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-b.id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mlockdb01k" {
  name        = "mlockdb01k"
  zone_id     = local.zones.zone_a.id
  platform_id = "standard-v2"
  fqdn        = "mlockdb01k.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 50
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = local.mlock_boot_disk_spec.size
      image_id = local.mlock_boot_disk_spec.image_id
      type_id  = local.mlock_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-a.id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mlock01f" {
  name        = "mlock01f"
  zone_id     = local.zones.zone_c.id
  platform_id = "standard-v2"
  fqdn        = "mlock01f.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 50
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = local.mlock_boot_disk_spec.size
      image_id = local.mlock_boot_disk_spec.image_id
      type_id  = local.mlock_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-c.id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mlock01h" {
  name        = "mlock01h"
  zone_id     = local.zones.zone_b.id
  platform_id = "standard-v2"
  fqdn        = "mlock01h.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 50
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = local.mlock_boot_disk_spec.size
      image_id = local.mlock_boot_disk_spec.image_id
      type_id  = local.mlock_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-b.id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mlock01k" {
  name        = "mlock01k"
  zone_id     = local.zones.zone_a.id
  platform_id = "standard-v2"
  fqdn        = "mlock01k.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 50
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = local.mlock_boot_disk_spec.size
      image_id = local.mlock_boot_disk_spec.image_id
      type_id  = local.mlock_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-a.id
    primary_v6_address {}
  }
}

resource "ycp_load_balancer_network_load_balancer" "mlock" {
  name      = "mlock-lb"
  region_id = var.region_id
  type      = "EXTERNAL"
  attached_target_group {
    target_group_id = ycp_load_balancer_target_group.mlock.id
    health_check {
      healthy_threshold   = 2
      interval            = "5s"
      name                = "mlock-api-hc"
      timeout             = "1s"
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
    name        = "mlock-api"
    protocol    = "TCP"
    external_address_spec {
      yandex_only = true
      ip_version  = "IPV6"
      address     = "2a0d:d6c0:0:1b::140"
    }
  }
}

resource "ycp_load_balancer_target_group" "mlock" {
  name      = "mlock"
  region_id = var.region_id
  target {
    address   = ycp_compute_instance.mlock01f.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.mlock01f.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.mlock01k.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.mlock01k.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.mlock01h.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.mlock01h.network_interface.0.subnet_id
  }
}

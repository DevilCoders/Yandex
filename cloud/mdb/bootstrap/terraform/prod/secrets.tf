resource "ycp_compute_instance" "mdb-secrets-prod01f" {
  name                      = "mdb-secrets-prod01f"
  zone_id                   = local.zones.zone_c.id
  platform_id               = "standard-v2"
  fqdn                      = "mdb-secrets-prod01f.yandexcloud.net"
  allow_stopping_for_update = true

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
      size     = 30
      image_id = "fd8jkvm5tese03u9c2ru"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_c.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mdb-secrets-prod01h" {
  name                      = "mdb-secrets-prod01h"
  zone_id                   = local.zones.zone_b.id
  platform_id               = "standard-v2"
  fqdn                      = "mdb-secrets-prod01h.yandexcloud.net"
  allow_stopping_for_update = true

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
      size     = 30
      image_id = "fd8jkvm5tese03u9c2ru"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_b.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mdb-secrets-prod01k" {
  name                      = "mdb-secrets-prod01k"
  zone_id                   = local.zones.zone_a.id
  platform_id               = "standard-v2"
  fqdn                      = "mdb-secrets-prod01k.yandexcloud.net"
  allow_stopping_for_update = true

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
      size     = 30
      image_id = "fd8jkvm5tese03u9c2ru"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_a.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "secrets-db-prod01f" {
  name                      = "secrets-db-prod01f"
  zone_id                   = local.zones.zone_c.id
  platform_id               = "standard-v2"
  fqdn                      = "secrets-db-prod01f.yandexcloud.net"
  allow_stopping_for_update = true

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
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = 60
      image_id = "fd8jkvm5tese03u9c2ru"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_c.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "secrets-db-prod01h" {
  name                      = "secrets-db-prod01h"
  zone_id                   = local.zones.zone_b.id
  platform_id               = "standard-v2"
  fqdn                      = "secrets-db-prod01h.yandexcloud.net"
  allow_stopping_for_update = true

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
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = 60
      image_id = "fd8jkvm5tese03u9c2ru"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_b.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "secrets-db-prod01k" {
  name                      = "secrets-db-prod01k"
  zone_id                   = local.zones.zone_a.id
  platform_id               = "standard-v2"
  fqdn                      = "secrets-db-prod01k.yandexcloud.net"
  allow_stopping_for_update = true

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
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = 60
      image_id = "fd8jkvm5tese03u9c2ru"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_a.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_load_balancer_target_group" "mdb-secrets" {
  name        = "mdb-secrets"
  region_id   = var.region_id
  description = "MDB Secrets api host"
  target {
    address   = ycp_compute_instance.mdb-secrets-prod01f.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.mdb-secrets-prod01f.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.mdb-secrets-prod01k.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.mdb-secrets-prod01k.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.mdb-secrets-prod01h.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.mdb-secrets-prod01h.network_interface.0.subnet_id
  }
}

resource "ycp_load_balancer_network_load_balancer" "mdb-secrets" {
  name        = "mdb-secrets-lb"
  region_id   = var.region_id
  type        = "EXTERNAL"
  description = "MDB Secrets balancer"
  attached_target_group {
    target_group_id = ycp_load_balancer_target_group.mdb-secrets.id
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
    name        = "mdb-secrets-api-tls"
    protocol    = "TCP"
    external_address_spec {
      yandex_only = true
      ip_version  = "IPV6"
      address     = "2a0d:d6c0:0:1b::27d"
    }
  }
}

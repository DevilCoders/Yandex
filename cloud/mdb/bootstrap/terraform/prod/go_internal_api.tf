locals {
  mdb_internal_api_boot_disk_spec = {
    size     = 30
    image_id = "fd8cdpnfbp3m4c2udtep"
    type_id  = "network-hdd"
  }
}

resource "ycp_compute_instance" "mdb-internal-api01f" {
  name        = "mdb-internal-api01f"
  zone_id     = local.zones.zone_c.id
  platform_id = "standard-v1"
  fqdn        = "mdb-internal-api01f.yandexcloud.net"

  disable_seccomp = true

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
    disk_spec {
      size     = local.mdb_internal_api_boot_disk_spec.size
      image_id = local.mdb_internal_api_boot_disk_spec.image_id
      type_id  = local.mdb_internal_api_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = local.zones.zone_c.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mdb-internal-api01h" {
  name        = "mdb-internal-api01h"
  zone_id     = local.zones.zone_b.id
  platform_id = "standard-v1"
  fqdn        = "mdb-internal-api01h.yandexcloud.net"

  disable_seccomp = true

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
    disk_spec {
      size     = local.mdb_internal_api_boot_disk_spec.size
      image_id = local.mdb_internal_api_boot_disk_spec.image_id
      type_id  = local.mdb_internal_api_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = local.zones.zone_b.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mdb-internal-api01k" {
  name        = "mdb-internal-api01k"
  zone_id     = local.zones.zone_a.id
  platform_id = "standard-v1"
  fqdn        = "mdb-internal-api01k.yandexcloud.net"

  disable_seccomp = true

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
    disk_spec {
      size     = local.mdb_internal_api_boot_disk_spec.size
      image_id = local.mdb_internal_api_boot_disk_spec.image_id
      type_id  = local.mdb_internal_api_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = local.zones.zone_a.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_load_balancer_target_group" "mdb-internal-api" {
  name      = "mdb-internal-api"
  region_id = var.region_id
  target {
    address   = ycp_compute_instance.mdb-internal-api01f.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.mdb-internal-api01f.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.mdb-internal-api01k.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.mdb-internal-api01k.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.mdb-internal-api01h.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.mdb-internal-api01h.network_interface.0.subnet_id
  }
}

resource "ycp_load_balancer_network_load_balancer" "mdb-internal-api" {
  name      = "mdb-internal-api-lb"
  region_id = var.region_id
  type      = "EXTERNAL"
  attached_target_group {
    target_group_id = ycp_load_balancer_target_group.mdb-internal-api.id
    health_check {
      healthy_threshold   = 2
      interval            = "5s"
      name                = "mdb-internal-api-hc"
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
    name        = "mdb-internal-api-lb-listener"
    protocol    = "TCP"
    external_address_spec {
      yandex_only = true
      ip_version  = "IPV6"
      address     = "2a0d:d6c0:0:1b::38c"
    }
  }
}

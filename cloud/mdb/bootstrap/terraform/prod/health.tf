resource "ycp_compute_instance" "health-dbaas01f" {
  name        = "health-dbaas01f"
  zone_id     = local.zones.zone_c.id
  platform_id = "standard-v1"
  fqdn        = "health-dbaas01f.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 16
    memory        = 16
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

resource "ycp_compute_instance" "health-dbaas01h" {
  name        = "health-dbaas01h"
  zone_id     = local.zones.zone_b.id
  platform_id = "standard-v1"
  fqdn        = "health-dbaas01h.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 16
    memory        = 16
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

resource "ycp_compute_instance" "health-dbaas01k" {
  name        = "health-dbaas01k"
  zone_id     = local.zones.zone_a.id
  platform_id = "standard-v1"
  fqdn        = "health-dbaas01k.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 16
    memory        = 16
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

resource "ycp_load_balancer_target_group" "mdb-health" {
  name      = "mdb-health"
  region_id = var.region_id
  target {
    address   = ycp_compute_instance.health-dbaas01f.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.health-dbaas01f.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.health-dbaas01k.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.health-dbaas01k.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.health-dbaas01h.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.health-dbaas01h.network_interface.0.subnet_id
  }
}

resource "ycp_load_balancer_network_load_balancer" "mdb-health" {
  name      = "mdb-health-lb"
  region_id = var.region_id
  type      = "EXTERNAL"
  attached_target_group {
    target_group_id = ycp_load_balancer_target_group.mdb-health.id
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
    name        = "mdb-health-api"
    protocol    = "TCP"
    external_address_spec {
      yandex_only = true
      ip_version  = "IPV6"
      address     = "2a0d:d6c0:0:1c::2c1"
    }
  }
}

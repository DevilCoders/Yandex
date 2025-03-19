resource "ycp_compute_instance" "mdb-dns01f" {
  name                      = "mdb-dns01f"
  zone_id                   = local.zones.zone_c.id
  platform_id               = "standard-v2"
  fqdn                      = "mdb-dns01f.yandexcloud.net"
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
    cores         = 4
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

resource "ycp_compute_instance" "mdb-dns01h" {
  name                      = "mdb-dns01h"
  zone_id                   = local.zones.zone_b.id
  platform_id               = "standard-v2"
  fqdn                      = "mdb-dns01h.yandexcloud.net"
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
    cores         = 4
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

resource "ycp_compute_instance" "mdb-dns01k" {
  name                      = "mdb-dns01k"
  zone_id                   = local.zones.zone_a.id
  platform_id               = "standard-v2"
  fqdn                      = "mdb-dns01k.yandexcloud.net"
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
    cores         = 4
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

resource "ycp_load_balancer_target_group" "mdb-dns" {
  name      = "mdb-dns"
  region_id = var.region_id
  target {
    address   = ycp_compute_instance.mdb-dns01f.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.mdb-dns01f.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.mdb-dns01k.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.mdb-dns01k.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.mdb-dns01h.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.mdb-dns01h.network_interface.0.subnet_id
  }
}

resource "ycp_load_balancer_network_load_balancer" "mdb-dns" {
  name      = "mdb-dns-lb"
  region_id = var.region_id
  type      = "EXTERNAL"
  attached_target_group {
    target_group_id = ycp_load_balancer_target_group.mdb-dns.id
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
    name        = "mdb-dns-api"
    protocol    = "TCP"
    external_address_spec {
      yandex_only = true
      ip_version  = "IPV6"
      address     = "2a0d:d6c0:0:1c::1ad"
    }
  }
}

resource "ycp_iam_service_account" "mdb-dns" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "mdb-dns"
  description        = "Manages DNS records for databases"
  service_account_id = "yc.mdb.dns"
}

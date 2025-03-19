resource "ycp_compute_instance" "health-dbaas-preprod01f" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "health-dbaas-preprod01f"
  zone_id                   = local.zones.zone_c.id
  platform_id               = "standard-v2"
  fqdn                      = "health-dbaas-preprod01f.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 30
      image_id = "fdvda8nslqgj4kl86h9f"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-c.id
    primary_v6_address {}
  }

  pci_topology_id = "PCI_TOPOLOGY_ID_UNSPECIFIED"
}

resource "ycp_compute_instance" "health-dbaas-preprod01k" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "health-dbaas-preprod01k"
  zone_id                   = local.zones.zone_a.id
  platform_id               = "standard-v2"
  fqdn                      = "health-dbaas-preprod01k.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 30
      image_id = "fdvda8nslqgj4kl86h9f"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-a.id
    primary_v6_address {}
  }

  pci_topology_id = "PCI_TOPOLOGY_ID_UNSPECIFIED"
}

resource "ycp_compute_instance" "health-dbaas-preprod01h" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "health-dbaas-preprod01h"
  zone_id                   = local.zones.zone_b.id
  platform_id               = "standard-v2"
  fqdn                      = "health-dbaas-preprod01h.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 30
      image_id = "fdv2sbduod8qobb2vm47"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-b.id
    primary_v6_address {}
  }

  pci_topology_id = "PCI_TOPOLOGY_ID_UNSPECIFIED"
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
    name        = "mdb-health-listener"
    protocol    = "TCP"
    external_address_spec {
      yandex_only = true
      ip_version  = "IPV6"
      address     = "2a0d:d6c0:0:ff1a::3bd"
    }
  }
}

resource "ycp_load_balancer_target_group" "mdb-health" {
  name      = "mdb-health"
  region_id = var.region_id
  target {
    address   = ycp_compute_instance.health-dbaas-preprod01f.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.health-dbaas-preprod01f.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.health-dbaas-preprod01k.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.health-dbaas-preprod01k.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.health-dbaas-preprod01h.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.health-dbaas-preprod01h.network_interface.0.subnet_id
  }
}

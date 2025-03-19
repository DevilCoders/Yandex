resource "ycp_compute_instance" "secrets-db-preprod01f" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "secrets-db-preprod01f"
  zone_id                   = local.zones.zone_c.id
  platform_id               = "standard-v2"
  fqdn                      = "secrets-db-preprod01f.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      name        = data.yandex_compute_disk.secrets-db-preprod01f_boot.name
      size        = data.yandex_compute_disk.secrets-db-preprod01f_boot.size
      image_id    = data.yandex_compute_disk.secrets-db-preprod01f_boot.image_id
      snapshot_id = data.yandex_compute_disk.secrets-db-preprod01f_boot.snapshot_id
      type_id     = data.yandex_compute_disk.secrets-db-preprod01f_boot.type
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-c.id
    primary_v6_address {}
  }

  pci_topology_id = "PCI_TOPOLOGY_ID_UNSPECIFIED"
}

resource "ycp_compute_instance" "secrets-db-preprod01h" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "secrets-db-preprod01h"
  zone_id                   = local.zones.zone_b.id
  platform_id               = "standard-v2"
  fqdn                      = "secrets-db-preprod01h.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk {
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

resource "ycp_compute_instance" "secrets-db-preprod01k" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "secrets-db-preprod01k"
  zone_id                   = local.zones.zone_a.id
  platform_id               = "standard-v2"
  fqdn                      = "secrets-db-preprod01k.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      name        = data.yandex_compute_disk.secrets-db-preprod01k_boot.name
      size        = data.yandex_compute_disk.secrets-db-preprod01k_boot.size
      image_id    = data.yandex_compute_disk.secrets-db-preprod01k_boot.image_id
      snapshot_id = data.yandex_compute_disk.secrets-db-preprod01k_boot.snapshot_id
      type_id     = data.yandex_compute_disk.secrets-db-preprod01k_boot.type
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-a.id
    primary_v6_address {}
  }

  pci_topology_id = "PCI_TOPOLOGY_ID_UNSPECIFIED"
}

data "yandex_compute_disk" "secrets-db-preprod01k_boot" {
  disk_id = "a7la1aulgcd83sjl3hcl"
}
data "yandex_compute_disk" "secrets-db-preprod01h_boot" {
  disk_id = "c8rbbmmhhucvd0obh28b"
}
data "yandex_compute_disk" "secrets-db-preprod01f_boot" {
  disk_id = "d9hls6ku8i9bu535eeva"
}

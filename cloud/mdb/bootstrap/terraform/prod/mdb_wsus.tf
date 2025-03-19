locals {
  wsus_boot_disk_spec = {
    size     = 30
    image_id = "fd8dbqp9ok83kchcblsu"
    type_id  = "network-hdd"
  }
}

resource "yandex_compute_disk" "wsus_prod01f_disk" {
  name = "wsus-prod01f-disk"
  type = "network-ssd"
  size = 100
  zone = "ru-central1-c"
}

resource "ycp_compute_instance" "mdb_wsus_prod01f" {
  name        = "mdb-wsus-prod01f"
  zone_id     = local.zones.zone_c.id
  platform_id = "standard-v2"
  fqdn        = "mdb-wsus-prod01f.yandexcloud.net"

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
      size     = local.wsus_boot_disk_spec.size
      image_id = local.wsus_boot_disk_spec.image_id
      type_id  = local.wsus_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-c.id
    primary_v6_address {}
  }

  secondary_disk {
    disk_id = yandex_compute_disk.wsus_prod01f_disk.id
  }
}

resource "yandex_compute_disk" "wsus_prod01h_disk" {
  name = "wsus-prod01h-disk"
  type = "network-ssd"
  size = 100
  zone = "ru-central1-b"
}

resource "ycp_compute_instance" "mdb_wsus_prod01h" {
  name        = "mdb-wsus-prod01h"
  zone_id     = local.zones.zone_b.id
  platform_id = "standard-v2"
  fqdn        = "mdb-wsus-prod01h.yandexcloud.net"

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
      size     = local.wsus_boot_disk_spec.size
      image_id = local.wsus_boot_disk_spec.image_id
      type_id  = local.wsus_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-b.id
    primary_v6_address {}
  }

  secondary_disk {
    disk_id = yandex_compute_disk.wsus_prod01h_disk.id
  }
}

resource "yandex_compute_disk" "wsus_prod01k_disk" {
  name = "wsus-prod01k-disk"
  type = "network-ssd"
  size = 100
  zone = "ru-central1-a"
}

resource "ycp_compute_instance" "mdb_wsus_prod01k" {
  name        = "mdb-wsus-prod01k"
  zone_id     = local.zones.zone_a.id
  platform_id = "standard-v2"
  fqdn        = "mdb-wsus-prod01k.yandexcloud.net"

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
      size     = local.wsus_boot_disk_spec.size
      image_id = local.wsus_boot_disk_spec.image_id
      type_id  = local.wsus_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-a.id
    primary_v6_address {}
  }

  secondary_disk {
    disk_id = yandex_compute_disk.wsus_prod01k_disk.id
  }
}


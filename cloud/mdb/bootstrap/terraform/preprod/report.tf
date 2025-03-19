resource "ycp_compute_instance" "mdb-report-preprod01h" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "mdb-report-preprod01h"
  zone_id                   = local.zones.zone_b.id
  platform_id               = "standard-v2"
  fqdn                      = "mdb-report-preprod01h.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 4
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = 30
      image_id = "fdv19ubn2bdfq62v5mop"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-b.id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mdb-report-preprod01k" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "mdb-report-preprod01k"
  zone_id                   = local.zones.zone_a.id
  platform_id               = "standard-v2"
  fqdn                      = "mdb-report-preprod01k.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 4
    memory        = 4
  }

  boot_disk {
    disk_spec {
      name        = data.yandex_compute_disk.mdb-report-preprod01k_boot.name
      size        = data.yandex_compute_disk.mdb-report-preprod01k_boot.size
      image_id    = data.yandex_compute_disk.mdb-report-preprod01k_boot.image_id
      snapshot_id = data.yandex_compute_disk.mdb-report-preprod01k_boot.snapshot_id
      type_id     = data.yandex_compute_disk.mdb-report-preprod01k_boot.type
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-a.id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mdb-report-preprod01f" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "mdb-report-preprod01f"
  zone_id                   = local.zones.zone_c.id
  platform_id               = "standard-v2"
  fqdn                      = "mdb-report-preprod01f.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 4
    memory        = 4
  }

  boot_disk {
    disk_spec {
      name        = data.yandex_compute_disk.mdb-report-preprod01f_boot.name
      size        = data.yandex_compute_disk.mdb-report-preprod01f_boot.size
      image_id    = data.yandex_compute_disk.mdb-report-preprod01f_boot.image_id
      snapshot_id = data.yandex_compute_disk.mdb-report-preprod01f_boot.snapshot_id
      type_id     = data.yandex_compute_disk.mdb-report-preprod01f_boot.type
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-c.id
    primary_v6_address {}
  }

  pci_topology_id = "PCI_TOPOLOGY_ID_UNSPECIFIED"
}

data "yandex_compute_disk" "mdb-report-preprod01f_boot" {
  disk_id = "d9hlie7rc620hrg59r4k"
}
data "yandex_compute_disk" "mdb-report-preprod01k_boot" {
  disk_id = "a7li04l66saems1kf41h"
}
data "yandex_compute_disk" "mdb-report-preprod01h_boot" {
  disk_id = "c8r70c7soo7sjsv79pac"
}

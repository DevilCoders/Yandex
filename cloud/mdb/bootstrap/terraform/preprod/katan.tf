resource "ycp_compute_instance" "katan-db-preprod01f" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name        = "katan-db-preprod01f"
  zone_id     = local.zones.zone_c.id
  platform_id = "standard-v2"
  fqdn        = "katan-db-preprod01f.cloud-preprod.yandex.net"

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      name        = data.yandex_compute_disk.katan-db-preprod01f_boot.name
      size        = data.yandex_compute_disk.katan-db-preprod01f_boot.size
      image_id    = data.yandex_compute_disk.katan-db-preprod01f_boot.image_id
      snapshot_id = data.yandex_compute_disk.katan-db-preprod01f_boot.snapshot_id
      type_id     = data.yandex_compute_disk.katan-db-preprod01f_boot.type
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-c.id
    primary_v6_address {}
  }

  disable_seccomp = true
}

resource "ycp_compute_instance" "katan-db-preprod01h" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name        = "katan-db-preprod01h"
  zone_id     = local.zones.zone_b.id
  platform_id = "standard-v2"
  fqdn        = "katan-db-preprod01h.cloud-preprod.yandex.net"

  resources {
    core_fraction = 100
    cores         = 2
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

  disable_seccomp = true
}

resource "ycp_compute_instance" "katan-db-preprod01k" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name        = "katan-db-preprod01k"
  zone_id     = local.zones.zone_a.id
  platform_id = "standard-v2"
  fqdn        = "katan-db-preprod01k.cloud-preprod.yandex.net"

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      name        = data.yandex_compute_disk.katan-db-preprod01k_boot.name
      size        = data.yandex_compute_disk.katan-db-preprod01k_boot.size
      image_id    = data.yandex_compute_disk.katan-db-preprod01k_boot.image_id
      snapshot_id = data.yandex_compute_disk.katan-db-preprod01k_boot.snapshot_id
      type_id     = data.yandex_compute_disk.katan-db-preprod01k_boot.type
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-a.id
    primary_v6_address {}
  }

  disable_seccomp = true
}


data "yandex_compute_disk" "katan-db-preprod01k_boot" {
  disk_id = "a7lgaf5599h9lscn32b9"
}
data "yandex_compute_disk" "katan-db-preprod01f_boot" {
  disk_id = "d9hsstc0okaodbrfiqcp"
}

resource "ycp_iam_service_account" "katan" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "katan"
  service_account_id = "yc.mdb.katan"
  description        = "service account for Katan service"
}

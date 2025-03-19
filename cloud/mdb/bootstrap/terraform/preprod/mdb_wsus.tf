locals {
  wsus_boot_disk_spec = {
    size     = 30
    image_id = "fdvbe764tr4ot7j72b20"
    type_id  = "network-hdd"
  }
}

resource "yandex_compute_disk" "wsus-preprod01f-disk" {
  name = "wsus-preprod01f-disk"
  type = "network-ssd"
  size = 100
  zone = "ru-central1-c"
}

resource "ycp_compute_instance" "mdb-wsus-preprod01f" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name        = "mdb-wsus-preprod01f"
  zone_id     = local.zones.zone_c.id
  platform_id = "standard-v2"
  fqdn        = "mdb-wsus-preprod01f.cloud-preprod.yandex.net"

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
    disk_id = yandex_compute_disk.wsus-preprod01f-disk.id
  }
}

resource "yandex_compute_disk" "wsus-preprod01h-disk" {
  name = "wsus-preprod01h-disk"
  type = "network-ssd"
  size = 100
  zone = "ru-central1-b"
}

resource "ycp_compute_instance" "mdb-wsus-preprod01h" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name        = "mdb-wsus-preprod01h"
  zone_id     = local.zones.zone_b.id
  platform_id = "standard-v2"
  fqdn        = "mdb-wsus-preprod01h.cloud-preprod.yandex.net"

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
    disk_id = yandex_compute_disk.wsus-preprod01h-disk.id
  }
}

resource "yandex_compute_disk" "wsus-preprod01k-disk" {
  name = "wsus-preprod01k-disk"
  type = "network-ssd"
  size = 100
  zone = "ru-central1-a"
}

resource "ycp_compute_instance" "mdb-wsus-preprod01k" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name        = "mdb-wsus-preprod01k"
  zone_id     = local.zones.zone_a.id
  platform_id = "standard-v2"
  fqdn        = "mdb-wsus-preprod01k.cloud-preprod.yandex.net"

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
    disk_id = yandex_compute_disk.wsus-preprod01k-disk.id
  }
}


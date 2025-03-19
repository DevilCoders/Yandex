locals {
  mlock_boot_disk_spec = {
    size     = 30
    image_id = "fdvp2dmt3098nnqdc4vr"
    type_id  = "network-hdd"
  }
}

resource "yandex_iam_service_account" "mlockmonitor" {
  name = "mlockmonitor"
}

resource "yandex_iam_service_account" "mlockworker" {
  name = "mlockworker"
}

resource "ycp_compute_instance" "mlockdb-preprod01f" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "mlockdb-preprod01f"
  zone_id                   = local.zones.zone_c.id
  platform_id               = "standard-v2"
  fqdn                      = "mlockdb-preprod01f.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = local.mlock_boot_disk_spec.size
      image_id = local.mlock_boot_disk_spec.image_id
      type_id  = local.mlock_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-c.id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mlockdb-preprod01h" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "mlockdb-preprod01h"
  zone_id                   = local.zones.zone_b.id
  platform_id               = "standard-v2"
  fqdn                      = "mlockdb-preprod01h.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = local.mlock_boot_disk_spec.size
      image_id = "fdv2sbduod8qobb2vm47"
      type_id  = local.mlock_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-b.id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mlockdb-preprod01k" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "mlockdb-preprod01k"
  zone_id                   = local.zones.zone_a.id
  platform_id               = "standard-v2"
  fqdn                      = "mlockdb-preprod01k.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = local.mlock_boot_disk_spec.size
      image_id = local.mlock_boot_disk_spec.image_id
      type_id  = local.mlock_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-a.id
    primary_v6_address {}
  }
}

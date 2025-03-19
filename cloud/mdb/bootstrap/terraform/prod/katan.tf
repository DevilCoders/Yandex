resource "ycp_compute_instance" "mdb-katandb01-rc1c" {
  name                      = "mdb-katandb01-rc1c"
  zone_id                   = local.zones.zone_c.id
  platform_id               = "standard-v2"
  fqdn                      = "mdb-katandb01-rc1c.yandexcloud.net"
  allow_stopping_for_update = true

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 20
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = 30
      image_id = "fd8st8fshnbt58mukkuf"
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-c.id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mdb-katandb01-rc1b" {
  name                      = "mdb-katandb01-rc1b"
  zone_id                   = local.zones.zone_b.id
  platform_id               = "standard-v2"
  fqdn                      = "mdb-katandb01-rc1b.yandexcloud.net"
  allow_stopping_for_update = true

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 20
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = 30
      image_id = "fd8st8fshnbt58mukkuf"
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-b.id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mdb-katandb01-rc1a" {
  name                      = "mdb-katandb01-rc1a"
  zone_id                   = local.zones.zone_a.id
  platform_id               = "standard-v2"
  fqdn                      = "mdb-katandb01-rc1a.yandexcloud.net"
  allow_stopping_for_update = true

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 20
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = 30
      image_id = "fd8st8fshnbt58mukkuf"
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-a.id
    primary_v6_address {}
  }
}

resource "ycp_iam_service_account" "katan" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "katan"
  service_account_id = "yc.mdb.katan"
  description        = "service account for Katan service"
}

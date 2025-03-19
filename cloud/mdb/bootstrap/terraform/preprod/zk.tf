resource "ycp_compute_instance" "zk-dbaas-preprod01f" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "zk-dbaas-preprod01f"
  zone_id                   = local.zones.zone_c.id
  platform_id               = "standard-v2"
  fqdn                      = "zk-dbaas-preprod01f.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 20
      image_id = "fdvktgsf0mcur6gshirj"
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-c.id
    primary_v6_address {}
  }

  pci_topology_id = "PCI_TOPOLOGY_ID_UNSPECIFIED"
}

resource "ycp_compute_instance" "zk-dbaas-preprod01h" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "zk-dbaas-preprod01h"
  zone_id                   = local.zones.zone_b.id
  platform_id               = "standard-v2"
  fqdn                      = "zk-dbaas-preprod01h.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size        = 30
      snapshot_id = "fdv7bqnp8grusdkuahum"
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-b.id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "zk-dbaas-preprod01k" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "zk-dbaas-preprod01k"
  zone_id                   = local.zones.zone_a.id
  platform_id               = "standard-v2"
  fqdn                      = "zk-dbaas-preprod01k.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size        = 30
      snapshot_id = "fdvttnec40iuhlj3qq7d"
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-a.id
    primary_v6_address {}
  }
}

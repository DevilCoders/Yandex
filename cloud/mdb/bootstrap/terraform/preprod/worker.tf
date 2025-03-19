resource "ycp_compute_instance" "worker-dbaas-preprod01f" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "worker-dbaas-preprod01f"
  zone_id                   = local.zones.zone_c.id
  platform_id               = "standard-v2"
  fqdn                      = "worker-dbaas-preprod01f.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 2
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 30
      image_id = "fdv4d57l23lo4cqvm3nf"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-c.id
    primary_v6_address {}
  }

  pci_topology_id = "PCI_TOPOLOGY_ID_UNSPECIFIED"
}

resource "ycp_compute_instance" "worker-dbaas-preprod01k" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "worker-dbaas-preprod01k"
  zone_id                   = local.zones.zone_a.id
  platform_id               = "standard-v2"
  fqdn                      = "worker-dbaas-preprod01k.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 2
  }

  boot_disk {
    disk_spec {
      size     = 30
      image_id = "fdvmga1dp56mbjo31hul"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-a.id
    primary_v6_address {}
  }

  pci_topology_id = "PCI_TOPOLOGY_ID_UNSPECIFIED"
}

resource "ycp_iam_service_account" "worker" {
  name               = "worker"
  service_account_id = "yc.mdb.worker"
  description        = "MDB-7080"
}

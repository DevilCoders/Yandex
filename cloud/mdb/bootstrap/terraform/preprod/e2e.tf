resource "ycp_compute_instance" "dbaas-e2e-preprod01k" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "dbaas-e2e-preprod01k"
  zone_id                   = local.zones.zone_a.id
  platform_id               = "standard-v2"
  fqdn                      = "dbaas-e2e-preprod01k.cloud-preprod.yandex.net"
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
      image_id = "fdvu4j4k36dt7p7fp6cd"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = "buc2gt91fs9e657k8bjr"
    primary_v4_address {}
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-a.id
    primary_v6_address {}
  }

  pci_topology_id = "PCI_TOPOLOGY_ID_UNSPECIFIED"
}

data "yandex_compute_disk" "dbaas-e2e-preprod01k_boot" {
  disk_id = "a7l89rtmpjtpdlg486jp"
}

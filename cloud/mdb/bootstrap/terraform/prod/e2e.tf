resource "ycp_compute_instance" "dbaas-e2e01k" {
  name                      = "dbaas-e2e01k"
  zone_id                   = local.zones.zone_a.id
  platform_id               = "standard-v2"
  fqdn                      = "dbaas-e2e01k.yandexcloud.net"
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
    auto_delete = false
    disk_spec {
      size     = 30
      image_id = "fd8hfplgh98va4ffc6r9"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = "e9bs8p3phrtdonteu5vq"
    primary_v4_address {}
  }

  network_interface {
    subnet_id = local.zones.zone_a.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mdb-ui-prod-01f" {
  name                      = "mdb-ui-prod-01f"
  zone_id                   = local.zones.zone_c.id
  platform_id               = "standard-v2"
  fqdn                      = "mdb-ui-01f.yandexcloud.net"
  allow_stopping_for_update = true

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    cores  = 2
    memory = 4
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 20
      image_id = "fd82t296sihk6rcaffag"
      type_id  = "network-hdd"
    }
  }

  placement_policy {
    host_group = "service"
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-c.id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mdb-ui-01h" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "mdb-ui-01h"
  zone_id                   = local.zones.zone_b.id
  platform_id               = "standard-v2"
  fqdn                      = "mdb-ui-01h.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    cores  = 2
    memory = 4
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 20
      image_id = "fdvlu41q397sfr6o2b3p"
      type_id  = "network-hdd"
    }
  }

  placement_policy {
    host_group = "service"
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-b.id
    primary_v6_address {}
  }
}

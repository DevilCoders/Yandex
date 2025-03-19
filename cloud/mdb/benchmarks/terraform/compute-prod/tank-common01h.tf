resource "ycp_compute_instance" "tank-common01h" {
  name                      = "tank-common01h"
  zone_id                   = local.zones.zone_b.id
  platform_id               = "standard-v2"
  fqdn                      = "tank-common01h.yandexcloud.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 8
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 30
      image_id = "fd8dbpm7kdor3lnb28ga"
      type_id  = "network-hdd"
    }
  }

  metadata = {
    "serial-port-enable" = "1"
  }

  network_interface {
    subnet_id = "e2lb9m60f8uu734h12iq"
    primary_v4_address { }
  }

  network_interface {
    subnet_id = local.zones.zone_b.subnet_id
    primary_v6_address { }
  }
}

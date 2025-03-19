resource "ycp_compute_instance" "mdb-report01f" {
  name        = "mdb-report01f"
  zone_id     = local.zones.zone_c.id
  platform_id = "standard-v2"
  fqdn        = "mdb-report01f.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size        = 30
      snapshot_id = "fd8q0ogf4rpba1kq5t3g"
      type_id     = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_c.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mdb-report01h" {
  name        = "mdb-report01h"
  zone_id     = local.zones.zone_b.id
  platform_id = "standard-v1"
  fqdn        = "mdb-report01h.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 8
  }

  boot_disk {
    disk_spec {
      size     = 30
      image_id = "fd88e038ojevas37moh1"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_b.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "mdb-report01k" {
  name        = "mdb-report01k"
  zone_id     = local.zones.zone_a.id
  platform_id = "standard-v2"
  fqdn        = "mdb-report01k.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size        = 30
      snapshot_id = "fd8n3koae01ee2k1efpf"
      type_id     = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_a.subnet_id
    primary_v6_address {}
  }
}

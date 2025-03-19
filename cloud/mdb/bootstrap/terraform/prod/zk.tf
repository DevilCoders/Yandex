resource "ycp_compute_instance" "zk-dbaas01f" {
  name        = "zk-dbaas01f"
  zone_id     = local.zones.zone_c.id
  platform_id = "standard-v2"
  fqdn        = "zk-dbaas01f.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 8
    memory        = 8
  }

  boot_disk {
    disk_spec {
      size        = 30
      snapshot_id = "fd8050vq90n55kof69mj"
      type_id     = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_c.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "zk-dbaas01h" {
  name        = "zk-dbaas01h"
  zone_id     = local.zones.zone_b.id
  platform_id = "standard-v1"
  fqdn        = "zk-dbaas01h.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 8
    memory        = 8
  }

  boot_disk {
    disk_spec {
      size     = 30
      image_id = "fd8oass1blo5oappg0lo"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_b.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "zk-dbaas01k" {
  name        = "zk-dbaas01k"
  zone_id     = local.zones.zone_a.id
  platform_id = "standard-v2"
  fqdn        = "zk-dbaas01k.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 8
    memory        = 8
  }

  boot_disk {
    disk_spec {
      size        = 30
      snapshot_id = "fd8svsne64et6ms07loc"
      type_id     = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_a.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "zk-dbaas02" {
  for_each = local.zones

  name        = "zk-dbaas02${each.value.letter}"
  zone_id     = each.value.id
  platform_id = "standard-v2"
  fqdn        = "zk-dbaas02${each.value.letter}.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 8
    memory        = 8
  }

  boot_disk {
    disk_spec {
      size     = 30
      image_id = "fd8190mko47lflj4cv7v"
    }
  }

  network_interface {
    subnet_id = each.value.subnet_id
    primary_v6_address {}
  }
}

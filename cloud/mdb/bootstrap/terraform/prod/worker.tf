resource "ycp_compute_instance" "worker-dbaas01f" {
  name        = "worker-dbaas01f"
  zone_id     = local.zones.zone_c.id
  platform_id = "standard-v1"
  fqdn        = "worker-dbaas01f.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 4
    memory        = 16
  }

  boot_disk {
    disk_spec {
      size     = 30
      image_id = "fd8tb1q4c03ntg5l76d7"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_c.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "worker-dbaas01h" {
  name        = "worker-dbaas01h"
  zone_id     = local.zones.zone_b.id
  platform_id = "standard-v1"
  fqdn        = "worker-dbaas01h.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 4
    memory        = 16
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 30
      image_id = "fd8dmei853vqamqse50s"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_b.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "worker-dbaas01k" {
  name        = "worker-dbaas01k"
  zone_id     = local.zones.zone_a.id
  platform_id = "standard-v1"
  fqdn        = "worker-dbaas01k.yandexcloud.net"

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 100
    cores         = 4
    memory        = 16
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = 30
      image_id = "fd8dmei853vqamqse50s"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_a.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_iam_service_account" "worker" {
  name               = "worker"
  service_account_id = "yc.mdb.worker"
  description        = "MDB-7080"
}

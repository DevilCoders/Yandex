locals {
  metadb_boot_disk_spec = {
    size     = 500
    image_id = "fd8vjfmsvs2ipm7qp979"
    type_id  = "network-ssd"
  }
}

resource "ycp_compute_instance" "meta-dbaas01f" {
  name                      = "meta-dbaas01f"
  zone_id                   = local.zones.zone_c.id
  platform_id               = "standard-v1"
  fqdn                      = "meta-dbaas01f.yandexcloud.net"
  allow_stopping_for_update = false

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
    memory        = 32
  }

  metadata = {
    user-data : local.deploy_userdata
    "serial-port-enable" = "0"
  }

  boot_disk {
    disk_spec {
      size     = local.metadb_boot_disk_spec.size
      image_id = local.metadb_boot_disk_spec.image_id
      type_id  = local.metadb_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = local.zones.zone_c.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "meta-dbaas01h" {
  name                      = "meta-dbaas01h"
  zone_id                   = local.zones.zone_b.id
  platform_id               = "standard-v1"
  fqdn                      = "meta-dbaas01h.yandexcloud.net"
  allow_stopping_for_update = false

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
    memory        = 32
  }

  metadata = {
    user-data : local.deploy_userdata
    "serial-port-enable" = "0"
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = local.metadb_boot_disk_spec.size
      image_id = local.metadb_boot_disk_spec.image_id
      type_id  = local.metadb_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = local.zones.zone_b.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "meta-dbaas01k" {
  name                      = "meta-dbaas01k"
  zone_id                   = local.zones.zone_a.id
  platform_id               = "standard-v1"
  fqdn                      = "meta-dbaas01k.yandexcloud.net"
  allow_stopping_for_update = false

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
    memory        = 32
  }

  metadata = {
    user-data : local.deploy_userdata
  }

  boot_disk {
    auto_delete = false
    disk_spec {
      size     = local.metadb_boot_disk_spec.size
      image_id = local.metadb_boot_disk_spec.image_id
      type_id  = local.metadb_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = local.zones.zone_a.subnet_id
    primary_v6_address {}
  }
}

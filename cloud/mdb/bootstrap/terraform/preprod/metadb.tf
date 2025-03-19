locals {
  metadb_boot_disk_spec = {
    size     = 300
    image_id = "fdvtk3pq1bs8o9v3ojd1"
    type_id  = "network-ssd"
  }
}

resource "ycp_compute_instance" "meta-dbaas-preprod01f" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "meta-dbaas-preprod01f"
  zone_id                   = local.zones.zone_c.id
  platform_id               = "standard-v2"
  fqdn                      = "meta-dbaas-preprod01f.cloud-preprod.yandex.net"
  allow_stopping_for_update = true


  resources {
    core_fraction = 100
    cores         = 2
    memory        = 8
  }

  metadata = {
    user-data : local.deploy_userdata
  }

  boot_disk {
    disk_spec {
      size     = local.metadb_boot_disk_spec.size
      image_id = local.metadb_boot_disk_spec.image_id
      type_id  = local.metadb_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-c.id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "meta-dbaas-preprod01h" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "meta-dbaas-preprod01h"
  zone_id                   = local.zones.zone_b.id
  platform_id               = "standard-v2"
  fqdn                      = "meta-dbaas-preprod01h.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 8
  }

  metadata = {
    user-data : local.deploy_userdata
  }

  boot_disk {
    disk_spec {
      size     = local.metadb_boot_disk_spec.size
      image_id = "fdv19ubn2bdfq62v5mop"
      type_id  = local.metadb_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-b.id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "meta-dbaas-preprod01k" {
  lifecycle {
    ignore_changes = [
      gpu_settings
    ]
  }

  name                      = "meta-dbaas-preprod01k"
  zone_id                   = local.zones.zone_a.id
  platform_id               = "standard-v2"
  fqdn                      = "meta-dbaas-preprod01k.cloud-preprod.yandex.net"
  allow_stopping_for_update = true

  resources {
    core_fraction = 100
    cores         = 2
    memory        = 8
  }

  metadata = {
    user-data : local.deploy_userdata
  }

  boot_disk {
    disk_spec {
      size     = local.metadb_boot_disk_spec.size
      image_id = local.metadb_boot_disk_spec.image_id
      type_id  = local.metadb_boot_disk_spec.type_id
    }
  }

  network_interface {
    subnet_id = ycp_vpc_subnet.dbaasexternalnets-ru-central1-a.id
    primary_v6_address {}
  }
}

resource "ycp_iam_service_account" "dataproc-manager" {
  service_account_id = "yc.dataproc.manager"
  name               = "dataproc-manager"
  description        = "Service account for dataproc-manager"
}

resource "yandex_iam_service_account_key" "dataproc-manager" {
  service_account_id = ycp_iam_service_account.dataproc-manager.id
  description        = "This key is used by dataproc-manager on compute VMs"
}

resource "ycp_iam_service_account" "control-plane-log-writer" {
  lifecycle {
    prevent_destroy = true
  }
  name        = "control-plane-log-writer"
  description = "Writes logs to cloud logging service from mdb control plane"
}

resource "yandex_resourcemanager_folder_iam_member" "control-plane-log-writer-role" {
  folder_id = var.folder_id
  member    = "serviceAccount:${ycp_iam_service_account.control-plane-log-writer.id}"
  role      = "logging.writer"
}

resource "ycp_compute_instance" "dataproc-manager01f" {
  name                      = "dataproc-manager01f"
  zone_id                   = local.zones.zone_c.id
  platform_id               = "standard-v2"
  fqdn                      = "dataproc-manager01f.yandexcloud.net"
  allow_stopping_for_update = true

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 50
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = 30
      image_id = "fd83kj6mshljr4nfpk0o"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_c.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "dataproc-manager01h" {
  name                      = "dataproc-manager01h"
  zone_id                   = local.zones.zone_b.id
  platform_id               = "standard-v2"
  fqdn                      = "dataproc-manager01h.yandexcloud.net"
  allow_stopping_for_update = true

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 50
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = 30
      image_id = "fd83kj6mshljr4nfpk0o"
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_b.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_compute_instance" "dataproc-manager01k" {
  name                      = "dataproc-manager01k"
  zone_id                   = local.zones.zone_a.id
  platform_id               = "standard-v2"
  fqdn                      = "dataproc-manager01k.yandexcloud.net"
  allow_stopping_for_update = true

  lifecycle {
    ignore_changes = [
      gpu_settings,
      pci_topology_id
    ]
    prevent_destroy = false
  }

  resources {
    core_fraction = 50
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      size     = 32
      image_id = "fd8if7ji89qae4kq7u1u" # yc compute image get-latest-from-family common-1if --profile mdb-prod-cp
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = local.zones.zone_a.subnet_id
    primary_v6_address {}
  }
}

resource "ycp_load_balancer_target_group" "dataproc-manager" {
  name      = "dataproc-manager"
  region_id = var.region_id
  target {
    address   = ycp_compute_instance.dataproc-manager01f.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.dataproc-manager01f.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.dataproc-manager01k.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.dataproc-manager01k.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.dataproc-manager01h.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.dataproc-manager01h.network_interface.0.subnet_id
  }
}

resource "ycp_load_balancer_network_load_balancer" "dataproc-manager" {
  name      = "dataproc-manager"
  region_id = var.region_id
  type      = "EXTERNAL"
  attached_target_group {
    target_group_id = ycp_load_balancer_target_group.dataproc-manager.id
    health_check {
      healthy_threshold   = 2
      interval            = "2s"
      name                = "dataproc-manager-tcp-healthcheck"
      timeout             = "1s"
      unhealthy_threshold = 2
      tcp_options {
        port = 443
      }
    }
  }
  listener_spec {
    port        = 443
    target_port = 443
    name        = "dataproc-manager"
    protocol    = "TCP"
    external_address_spec {
      yandex_only = true
      ip_version  = "IPV6"
      address     = "2a0d:d6c0:0:1b::12a"
    }
  }
}

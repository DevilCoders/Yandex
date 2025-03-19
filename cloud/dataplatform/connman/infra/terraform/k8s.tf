resource "yandex_container_registry" "cm-container-registry" {
  name = "cm-container-registry"
}

resource "yandex_kms_symmetric_key" "cm-k8s-key" {
  name              = "cm-k8s-key"
  default_algorithm = "AES_256"
}

resource "yandex_kubernetes_cluster" "cm-k8s-cluster" {
  name = "cm-k8s-cluster"

  network_id               = yandex_vpc_network.cm-net.id
  cluster_ipv4_range       = "10.1.0.0/16"
  cluster_ipv6_range       = "fc00::/96"
  node_ipv4_cidr_mask_size = 24
  service_ipv4_range       = "10.2.0.0/16"
  service_ipv6_range       = "fc01::/112"

  service_account_id      = yandex_iam_service_account.cm-admin.id
  node_service_account_id = yandex_iam_service_account.cm-admin.id
  depends_on = [
    yandex_resourcemanager_folder_iam_member.cm-admin-editor,
    yandex_resourcemanager_folder_iam_member.cm-admin-puller
  ]

  release_channel = "RAPID"

  kms_provider {
    key_id = yandex_kms_symmetric_key.cm-k8s-key.id
  }

  master {
    version   = "1.20"
    public_ip = true

    maintenance_policy {
      auto_upgrade = false
    }

    regional {
      region = "ru-central1"

      dynamic "location" {
        for_each = local.zones
        content {
          zone      = ycp_vpc_subnet.cm-subnet[location.value].zone_id
          subnet_id = ycp_vpc_subnet.cm-subnet[location.value].id
        }
      }
    }
  }
}

resource "yandex_kubernetes_node_group" "cm-k8s-node-group" {
  cluster_id = yandex_kubernetes_cluster.cm-k8s-cluster.id
  name       = "cm-k8s-node-group"
  version    = "1.20"

  instance_template {
    platform_id = "standard-v2"

    network_interface {
      nat        = false
      subnet_ids = [for x in local.zones : ycp_vpc_subnet.cm-subnet[x].id]
      ipv6       = true
    }

    resources {
      memory = 4
      cores  = 2
    }

    boot_disk {
      type = "network-hdd"
      size = 96
    }

    scheduling_policy {
      preemptible = false
    }
  }

  scale_policy {
    fixed_scale {
      size = 3
    }
  }

  allocation_policy {
    dynamic "location" {
      for_each = local.zones
      content {
        zone = location.value
      }

    }
  }

  maintenance_policy {
    auto_upgrade = false
    auto_repair  = true
  }
}

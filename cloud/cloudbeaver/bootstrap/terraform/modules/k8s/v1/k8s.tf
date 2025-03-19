resource "yandex_kubernetes_cluster" "k8s" {
  release_channel         = "RAPID"
  name                    = var.cluster_name
  description             = "CloudBeaver k8s cluster"
  network_id              = var.network_id
  node_service_account_id = yandex_iam_service_account.k8s_node.id
  service_account_id      = yandex_iam_service_account.k8s_cluster.id
  cluster_ipv4_range      = var.cluster_ipv4_range
  cluster_ipv6_range      = var.cluster_ipv6_range
  service_ipv4_range      = var.service_ipv4_range
  service_ipv6_range      = var.service_ipv6_range
  master {
    version   = "1.21"
    public_ip = true
    security_group_ids = [
      yandex_vpc_security_group.k8s-main-sg.id,
      yandex_vpc_security_group.k8s-master-whitelist.id
    ]
    regional {
      region = "ru-central1"
      dynamic "location" {
        for_each = var.locations
        content {
          subnet_id = location.value["subnet_id"]
          zone      = location.value["zone"]
        }
      }
    }
  }
}

resource "yandex_kubernetes_node_group" "this" {

  cluster_id = yandex_kubernetes_cluster.k8s.id
  instance_template {
    platform_id = "standard-v2"
    network_interface {
      ipv4       = true
      ipv6       = true
      nat        = false
      subnet_ids = var.locations[*].subnet_id
      security_group_ids = [
        yandex_vpc_security_group.k8s-main-sg.id,
        yandex_vpc_security_group.k8s-public-services.id,
        yandex_vpc_security_group.k8s-nodes-ssh-access.id,
      ]
    }

    resources {
      memory        = var.k8s_node_group.memory
      cores         = var.k8s_node_group.cores
      core_fraction = var.k8s_node_group.core_fraction
    }

    boot_disk {
      type = "network-hdd"
      size = 64
    }

    scheduling_policy {
      preemptible = false
    }

  }

  scale_policy {
    fixed_scale {
      size = var.k8s_node_group.size
    }
  }

  allocation_policy {
    dynamic "location" {
      for_each = var.locations
      content {
        zone = location.value["zone"]
      }
    }
  }

  maintenance_policy {
    auto_upgrade = true
    auto_repair  = true
  }

  depends_on = [
    yandex_resourcemanager_folder_iam_member.k8s_cluster_editor,
  ]
}

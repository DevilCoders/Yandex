resource "yandex_vpc_network" "mk8s-net" {
  name = "mk8s-net"
}

resource "yandex_vpc_subnet" "mk8s-subnet-a" {
  name = "mk8s-ru-central1-a-subnet"
  v4_cidr_blocks = ["10.1.0.0/24"]
  zone           = "ru-central1-a"
  network_id     = yandex_vpc_network.mk8s-net.id
}

resource "yandex_iam_service_account" "mk8s-sa" {
  name        = "mk8s-sa"
  description = "service account to manage VMs"
}

resource "yandex_resourcemanager_folder_iam_binding" "mk8s-editor" {
  folder_id = local.folder_id
  role = "editor"

  members = [
    "serviceAccount:${yandex_iam_service_account.mk8s-sa.id}"
  ]
}

resource "yandex_container_registry" "default" {
  name = "default"
}

resource "yandex_iam_service_account" "mk8s-image-puller-sa" {
  name        = "mk8s-image-puller-sa"
  description = "service account for pulling docker images from CR"
}

resource "yandex_container_registry_iam_binding" "mk8s-image-puller" {
  registry_id = yandex_container_registry.default.id
  role        = "container-registry.images.puller"

  members = [
    "serviceAccount:${yandex_iam_service_account.mk8s-image-puller-sa.id}"
  ]
}

resource "yandex_kubernetes_cluster" "mk8s-1" {
  network_id = yandex_vpc_network.mk8s-net.id
  name = "mk8s-1"
  master {
    #version = "1.15"
    zonal {
      zone      = yandex_vpc_subnet.mk8s-subnet-a.zone
      subnet_id = yandex_vpc_subnet.mk8s-subnet-a.id
    }

    public_ip = true

    maintenance_policy {
      auto_upgrade = false
    }
  }

  service_account_id      = yandex_iam_service_account.mk8s-sa.id
  node_service_account_id = yandex_iam_service_account.mk8s-image-puller-sa.id

  release_channel = "RAPID"
  network_policy_provider = "CALICO"

}

resource "yandex_kubernetes_node_group" "working-nodes" {
  for_each = local.working-nodes-groups
  name = "${each.key}-working-nodes"
  cluster_id  = yandex_kubernetes_cluster.mk8s-1.id

  node_labels = {
    "dummy" = each.key
  }
  instance_template {
    platform_id = "standard-v2"

    network_interface {
      nat                = true
      subnet_ids         = [yandex_vpc_subnet.mk8s-subnet-a.id]
    }

    resources {
      memory = 2
      cores  = 2
      core_fraction = 5
    }

    boot_disk {
      type = "network-hdd"
      size = 64
    }

    scheduling_policy {
      preemptible = true
    }
  }

  scale_policy {
    fixed_scale {
      size = each.value
    }
  }

  allocation_policy {
    location {
      zone = "ru-central1-a"
    }
  }

  maintenance_policy {
    auto_upgrade = false
    auto_repair  = false
 }
}


resource "yandex_container_registry" "main-image-registry" {
  name      = "main-image-registry"
  folder_id = var.kubernetes_dataplane_clusters_folder_id
}

variable "kubernetes_service_network_id" {
  type    = string
  default = "c64fs7250h41729o2ctn"
}

variable "default_zone" {
  type    = string
  default = "ru-central1-a"
}

variable "default_zone_subnet_id" {
  type    = string
  default = "bucd0genetip31dsb8bg"
}

resource "ycp_iam_service_account" "yc-metastore-kubermaster" {
  name               = "yc-metastore-kubermaster"
  description        = "MDB-18533: service account for dataplane kubernetes clusters master nodes"
  service_account_id = "yc.metastore.kubermaster"
  folder_id          = var.kubernetes_dataplane_clusters_folder_id
}

resource "ycp_iam_service_account" "yc-metastore-kubernode" {
  name               = "yc-metastore-kubernode"
  description        = "MDB-18533: service account for dataplane kubernetes clusters worker nodes"
  service_account_id = "yc.metastore.kubernode"
  folder_id          = var.kubernetes_dataplane_clusters_folder_id
}

resource "ycp_iam_service_account" "yc-metastore-image-pusher" {
  name               = "yc-metastore-image-pusher"
  description        = "MDB-18533: service account for pushing metastore Docker images to registry"
  service_account_id = "yc.metastore.image.pusher"
  folder_id          = var.kubernetes_dataplane_clusters_folder_id
}

resource "yandex_mdb_postgresql_cluster" "metastore-dataplane-db-cluster-1" {
  folder_id = var.kubernetes_dataplane_clusters_folder_id

  labels = {
    mdb-auto-purge : "off"
  }
  environment = "PRODUCTION"
  name        = "metastore-dataplane-db-1"
  network_id  = var.kubernetes_service_network_id

  config {
    version = 13
    resources {
      disk_type_id       = "network-ssd"
      disk_size          = 10
      resource_preset_id = "s2.micro"
    }
    postgresql_config = {
      max_connections = 400
    }
  }

  database {
    name  = "empty"
    owner = "empty"
  }
  host {
    subnet_id = var.default_zone_subnet_id
    zone      = var.default_zone
  }
  user {
    name     = "empty"
    password = "emptyempty"
    permission {
      database_name = "empty"
    }

  }
}

resource "yandex_kubernetes_cluster" "metastore-dataplane-k8s-cluster-1" {
  folder_id = var.kubernetes_dataplane_clusters_folder_id

  network_id         = var.kubernetes_service_network_id
  cluster_ipv4_range = "10.34.0.0/16"
  service_ipv4_range = "10.35.0.0/16"

  # needed for nodes with 2 network interfaces
  release_channel = "RAPID"

  master {
    version = "1.21"

    zonal {
      zone      = var.default_zone
      subnet_id = var.default_zone_subnet_id
    }

    # TODO remove public IP address when CLOUD-88912 is finished
    public_ip = true
  }

  service_account_id      = ycp_iam_service_account.yc-metastore-kubermaster.id
  node_service_account_id = ycp_iam_service_account.yc-metastore-kubernode.id
  depends_on = [
    yandex_resourcemanager_folder_iam_binding.editor,
    yandex_resourcemanager_folder_iam_binding.vpc-admin,
    yandex_resourcemanager_folder_iam_binding.vpc-user,
    yandex_resourcemanager_folder_iam_binding.images-puller
  ]
}

resource "yandex_resourcemanager_folder_iam_binding" "editor" {
  # Service account to be assigned "editor" role.
  folder_id = var.kubernetes_dataplane_clusters_folder_id
  role      = "editor"
  members = [
    "serviceAccount:${ycp_iam_service_account.yc-metastore-kubermaster.id}"
  ]
}

resource "yandex_resourcemanager_folder_iam_binding" "vpc-user" {
  # Service account to be assigned "editor" role.
  folder_id = var.kubernetes_dataplane_clusters_folder_id
  role      = "vpc.user"
  members = [
    "serviceAccount:${ycp_iam_service_account.yc-metastore-kubermaster.id}"
  ]
}

resource "yandex_resourcemanager_folder_iam_binding" "vpc-admin" {
  # Service account to be assigned "editor" role.
  folder_id = var.kubernetes_dataplane_clusters_folder_id
  role      = "vpc.admin"
  members = [
    "serviceAccount:${ycp_iam_service_account.yc-metastore-kubermaster.id}"
  ]
}

resource "yandex_resourcemanager_folder_iam_binding" "images-puller" {
  # Service account to be assigned "container-registry.images.puller" role.
  folder_id = var.kubernetes_dataplane_clusters_folder_id
  role      = "container-registry.images.puller"
  members = [
    "serviceAccount:${ycp_iam_service_account.yc-metastore-kubernode.id}"
  ]
}

resource "yandex_resourcemanager_folder_iam_binding" "yc-metastore-image-pusher" {
  # Service account to be assigned "container-registry.images.puller" role.
  folder_id = var.kubernetes_dataplane_clusters_folder_id
  role      = "container-registry.images.pusher"
  members = [
    "serviceAccount:${ycp_iam_service_account.yc-metastore-image-pusher.id}"
  ]
}

resource "yandex_resourcemanager_folder_iam_binding" "yc-metastore-image-pusher-viewer" {
  # Service account to be assigned "container-registry.images.puller" role.
  folder_id = var.kubernetes_dataplane_clusters_folder_id
  role      = "container-registry.images.viewer"
  members = [
    "serviceAccount:${ycp_iam_service_account.yc-metastore-image-pusher.id}"
  ]
}

resource "yandex_container_registry_iam_binding" "pusher" {
  registry_id = yandex_container_registry.main-image-registry.id
  role        = "container-registry.images.pusher"

  members = [
    "serviceAccount:${ycp_iam_service_account.yc-metastore-image-pusher.id}"
  ]

  depends_on = [
    yandex_container_registry.main-image-registry,
  ]
}

resource "ycp_vpc_network" "mdb-metastore-dualstack-nets" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id = var.kubernetes_dataplane_clusters_folder_id
  name      = "mdb-metastore-dualstack-nets"
}

resource "ycp_vpc_subnet" "mdb-metastore-dualstack-nets-ru-central1-a" {
  lifecycle {
    prevent_destroy = true
  }
  v6_cidr_blocks = ["2a02:6b8:c0e:501:0:fcb3::/112"]
  v4_cidr_blocks = ["10.211.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.kubernetes_dataplane_clusters_folder_id
  name       = "mdb-metastore-dualstack-nets-ru-central1-a"
  network_id = ycp_vpc_network.mdb-metastore-dualstack-nets.id
  zone_id    = "ru-central1-a"
}

resource "ycp_vpc_subnet" "mdb-metastore-dualstack-nets-ru-central1-b" {
  lifecycle {
    prevent_destroy = false
  }
  v6_cidr_blocks = ["2a02:6b8:c02:901:0:fcb3::/112"]
  v4_cidr_blocks = ["10.212.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.kubernetes_dataplane_clusters_folder_id
  name       = "mdb-metastore-dualstack-nets-ru-central1-b"
  network_id = ycp_vpc_network.mdb-metastore-dualstack-nets.id
  zone_id    = "ru-central1-b"
}

resource "ycp_vpc_subnet" "mdb-metastore-dualstack-nets-ru-central1-c" {
  lifecycle {
    prevent_destroy = false
  }
  v6_cidr_blocks = ["2a02:6b8:c03:501:0:fcb3::/112"]
  v4_cidr_blocks = ["10.213.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.kubernetes_dataplane_clusters_folder_id
  name       = "mdb-metastore-dualstack-nets-ru-central1-c"
  network_id = ycp_vpc_network.mdb-metastore-dualstack-nets.id
  zone_id    = "ru-central1-c"
}

resource "ycp_vpc_network" "mdb_kubernetes_infra_test_dualstack_nets" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id = var.kubernetes_infra_test_folder_id
  name      = "mdb-kubernetes-infra-test-dualstack-nets"
}

resource "ycp_vpc_subnet" "mdb_kubernetes_infra_test_dualstack_nets-ru-central1-a" {
  lifecycle {
    prevent_destroy = true
  }
  v6_cidr_blocks = ["2a02:6b8:c0e:501:0:fca9::/112"]
  v4_cidr_blocks = ["10.111.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.kubernetes_infra_test_folder_id
  name       = "mdb-kubernetes-infra-test-dualstack-nets-ru-central1-a"
  network_id = ycp_vpc_network.mdb_kubernetes_infra_test_dualstack_nets.id
  zone_id    = "ru-central1-a"
}

resource "ycp_vpc_subnet" "mdb_kubernetes_infra_test_dualstacknets-nets-ru-central1-b" {
  lifecycle {
    prevent_destroy = false
  }
  v6_cidr_blocks = ["2a02:6b8:c02:901:0:fca9::/112"]
  v4_cidr_blocks = ["10.112.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.kubernetes_infra_test_folder_id
  name       = "mdb-kubernetes-infra-test-dualstack-nets-ru-central1-b"
  network_id = ycp_vpc_network.mdb_kubernetes_infra_test_dualstack_nets.id
  zone_id    = "ru-central1-b"
}

resource "ycp_vpc_subnet" "mdb_kubernetes_infra_test_dualstack_nets-ru-central1-c" {
  lifecycle {
    prevent_destroy = false
  }
  v6_cidr_blocks = ["2a02:6b8:c03:501:0:fca9::/112"]
  v4_cidr_blocks = ["10.113.0.0/16"]
  extra_params {
    export_rts  = ["65533:666"]
    hbf_enabled = true
    import_rts  = ["65533:776"]
    rpf_enabled = false
  }
  egress_nat_enable = true

  folder_id  = var.kubernetes_infra_test_folder_id
  name       = "mdb-kubernetes-infra-test-dualstack-nets-ru-central1-c"
  network_id = ycp_vpc_network.mdb_kubernetes_infra_test_dualstack_nets.id
  zone_id    = "ru-central1-c"
}

resource "yandex_mdb_postgresql_cluster" "metastore-infratest-db-cluster-1" {
  folder_id = var.kubernetes_infra_test_folder_id

  labels = {
    mdb-auto-purge : "off"
  }
  environment = "PRODUCTION"
  name        = "metastore-dataplane-db-1"
  network_id  = ycp_vpc_network.mdb_kubernetes_infra_test_dualstack_nets.id

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
    subnet_id = ycp_vpc_subnet.mdb_kubernetes_infra_test_dualstacknets-nets-ru-central1-b.id
    zone      = ycp_vpc_subnet.mdb_kubernetes_infra_test_dualstacknets-nets-ru-central1-b.zone_id
  }
  user {
    name     = "empty"
    password = "emptyempty"
    permission {
      database_name = "empty"
    }

  }
}

resource "yandex_kubernetes_cluster" "metastore-infratest-k8s-cluster-1" {
  folder_id = var.kubernetes_infra_test_folder_id
  name      = "metastore-infratest-k8s-cluster-1"

  network_id         = ycp_vpc_network.mdb_kubernetes_infra_test_dualstack_nets.id
  cluster_ipv4_range = "10.14.0.0/16"
  service_ipv4_range = "10.15.0.0/16"

  # needed for nodes with 2 network interfaces
  release_channel = "RAPID"

  master {
    version = "1.21"

    zonal {
      zone      = ycp_vpc_subnet.mdb_kubernetes_infra_test_dualstacknets-nets-ru-central1-b.zone_id
      subnet_id = ycp_vpc_subnet.mdb_kubernetes_infra_test_dualstacknets-nets-ru-central1-b.id
    }

    # TODO remove public IP address when CLOUD-88912 is finished
    public_ip = true
  }

  service_account_id      = ycp_iam_service_account.yc-metastore-infratest-kubermaster.id
  node_service_account_id = ycp_iam_service_account.yc-metastore-infratest-kubernode.id
  depends_on = [
    yandex_resourcemanager_folder_iam_binding.yc-metastore-infratest-kubermaster-editor,
    yandex_resourcemanager_folder_iam_binding.yc-metastore-infratest-kubermaster-vpc-admin,
    yandex_resourcemanager_folder_iam_binding.yc-metastore-infratest-kubermaster-vpc-user,
    yandex_resourcemanager_folder_iam_binding.yc-metastore-infratest-kubernode-images-puller
  ]
}

resource "ycp_iam_service_account" "yc-metastore-infratest-kubermaster" {
  name        = "yc-metastore-infratest-kubermaster"
  description = "MDB-18533: service account for dataplane kubernetes clusters master nodes"
  folder_id   = var.kubernetes_infra_test_folder_id
}

resource "ycp_iam_service_account" "yc-metastore-infratest-kubernode" {
  name        = "yc-metastore-infratest-kubernode"
  description = "MDB-18533: service account for dataplane kubernetes clusters worker nodes"
  folder_id   = var.kubernetes_infra_test_folder_id
}

resource "yandex_resourcemanager_folder_iam_binding" "yc-metastore-infratest-kubermaster-editor" {
  # Service account to be assigned "editor" role.
  folder_id = var.kubernetes_infra_test_folder_id
  role      = "editor"
  members = [
    "serviceAccount:${ycp_iam_service_account.yc-metastore-infratest-kubermaster.id}"
  ]
}

resource "yandex_resourcemanager_folder_iam_binding" "yc-metastore-infratest-kubermaster-vpc-user" {
  # Service account to be assigned "editor" role.
  folder_id = var.kubernetes_infra_test_folder_id
  role      = "vpc.user"
  members = [
    "serviceAccount:${ycp_iam_service_account.yc-metastore-infratest-kubermaster.id}"
  ]
}

resource "yandex_resourcemanager_folder_iam_binding" "yc-metastore-infratest-kubermaster-vpc-admin" {
  # Service account to be assigned "editor" role.
  folder_id = var.kubernetes_infra_test_folder_id
  role      = "vpc.admin"
  members = [
    "serviceAccount:${ycp_iam_service_account.yc-metastore-infratest-kubermaster.id}"
  ]
}

resource "yandex_resourcemanager_folder_iam_binding" "yc-metastore-infratest-kubernode-images-puller" {
  # Service account to be assigned "container-registry.images.puller" role.
  folder_id = var.kubernetes_infra_test_folder_id
  role      = "container-registry.images.puller"
  members = [
    "serviceAccount:${ycp_iam_service_account.yc-metastore-infratest-kubernode.id}"
  ]
}

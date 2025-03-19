resource "ycp_iam_service_account" "yc_vpc_load_test_ig_sa" {
  lifecycle {
    prevent_destroy = true
  }

  folder_id          = var.folder_id
  service_account_id = "yc.vpc.vpc.load.test.ig.sa"
  name               = "yc-vpc-vpc-load-test-ig-sa"
}

resource "ycp_iam_service_account" "yc_vpc_load_test_node_sa" {
  lifecycle {
    prevent_destroy = true
  }

  folder_id          = var.folder_id
  service_account_id = "yc.vpc.vpc.load.test.node.sa"
  name               = "yc-vpc-vpc-load-test-node-sa"
}

resource "ycp_iam_service_account" "yc_vpc_load_test_hopper_sa" {
  lifecycle {
    prevent_destroy = true
  }

  folder_id          = var.folder_id
  service_account_id = "yc.vpc.load.test.hopper.sa"
  name               = "yc-vpc-load-test-hopper-sa"
}

resource "yandex_resourcemanager_folder_iam_binding" "yc_vpc_load_test_editor_sa" {
  folder_id = var.folder_id
  role = "editor"
  members = [
    "serviceAccount:${ycp_iam_service_account.yc_vpc_load_test_ig_sa.id}",
    "serviceAccount:${ycp_iam_service_account.yc_vpc_load_test_hopper_sa.id}",
  ]
}

resource "yandex_resourcemanager_folder_iam_binding" "images_puller" {
  folder_id = var.folder_id
  role = "container-registry.images.puller"
  members = [
    "serviceAccount:${ycp_iam_service_account.yc_vpc_load_test_node_sa.id}",
  ]
}


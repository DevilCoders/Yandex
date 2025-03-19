resource "ycp_iam_service_account" "yc_vpc_accounting_ig_sa" {
  lifecycle {
    prevent_destroy = true
  }

  folder_id          = var.folder_id
  service_account_id = "yc.vpc.accounting.ig.sa"
  name               = "yc-vpc-accounting-ig-sa"
}

resource "ycp_iam_service_account" "yc_vpc_accounting_hopper_sa" {
  lifecycle {
    prevent_destroy = true
  }

  folder_id          = var.folder_id
  service_account_id = "yc.vpc.accounting.hopper.sa"
  name               = "yc-vpc-accounting-hopper-sa"
}

resource "yandex_resourcemanager_folder_iam_binding" "yc_vpc_accounting_editor_sa" {
  folder_id = var.folder_id
  role      = "editor"
  members = [
    "serviceAccount:${ycp_iam_service_account.yc_vpc_accounting_ig_sa.id}",
    "serviceAccount:${ycp_iam_service_account.yc_vpc_accounting_hopper_sa.id}",
  ]
}

resource "ycp_iam_service_account" "yc_vpc_antifraud_sa" {
  lifecycle {
    prevent_destroy = true
  }

  folder_id          = var.folder_id
  service_account_id = "yc.vpc.accounting.antifraud.sa"
  name               = "yc-vpc-accounting-antifraud-sa"
}

resource "ycp_iam_service_account" "yc_vpc_billing_sa" {
  lifecycle {
    prevent_destroy = true
  }

  folder_id          = var.folder_id
  service_account_id = "yc.vpc.accounting.billing.sa"
  name               = "yc-vpc-accounting-billing-sa"
}

resource "ycp_iam_service_account" "yc_vpc_mirror_sa" {
  lifecycle {
    prevent_destroy = true
  }

  folder_id          = var.folder_id
  service_account_id = "yc.vpc.accounting.mirror.sa"
  name               = "yc-vpc-accounting-mirror-sa"
}

// Allow service account read journald logs for mirroring to Yandex (CLOUD-98518)
resource "ycp_resource_manager_folder_iam_member" "tf-lb-producer-yds-reader" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id = var.folder_id
  member    = "serviceAccount:${ycp_iam_service_account.yc_vpc_billing_sa.id}"
  role      = "yds.viewer"
}

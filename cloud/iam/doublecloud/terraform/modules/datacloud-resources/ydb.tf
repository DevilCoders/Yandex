locals {
  ydb_service_project_id = "yc.ydb.service"
  ydb_ydbc_project_id = "yc.ydb.ydbc"
}

variable "ydb_admin_sa_public_key" {
  type = string
}

variable "ydb_viewer_sa_public_key" {
  type = string
}

variable "ydb_controlplane_sa_public_key" {
  type = string
}

variable "yds_kinesis_proxy_sa_public_key" {
  type = string
}

resource "ycp_resource_manager_cloud" "ydb_service_cloud" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id        = local.ydb_service_project_id
  name            = "ydb-service"
  organization_id = "yc.organization-manager.yandex"
}

resource "ycp_resource_manager_folder" "ydb_service_folder" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id  = ycp_resource_manager_cloud.ydb_service_cloud.id
  folder_id = local.ydb_service_project_id
  name      = "ydb-service"
}

resource "ycp_iam_service_account" "ydb_admin_sa" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = ycp_resource_manager_folder.ydb_service_folder.id
  service_account_id = "yc.ydb.adminSA"
  name               = "ydb-admin-sa"
}

resource "ycp_iam_key" "ydb_admin_key" {
  lifecycle {
    prevent_destroy = true
  }
  key_id             = ycp_iam_service_account.ydb_admin_sa.id
  service_account_id = ycp_iam_service_account.ydb_admin_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.ydb_admin_sa_public_key
}

resource "ycp_iam_service_account" "ydb_viewer_sa" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = ycp_resource_manager_folder.ydb_service_folder.id
  service_account_id = "yc.ydb.viewerSA"
  name               = "ydb-viewer-sa"
}

resource "ycp_iam_key" "ydb_viewer_sa_key" {
  lifecycle {
    prevent_destroy = true
  }
  key_id             = ycp_iam_service_account.ydb_viewer_sa.id
  service_account_id = ycp_iam_service_account.ydb_viewer_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.ydb_viewer_sa_public_key
}

resource "ycp_iam_service_account" "yds_kinesis_proxy_sa" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = ycp_resource_manager_folder.ydb_service_folder.id
  service_account_id = "yc.yds.kinesisProxySA"
  name               = "yds-kinesis-proxy-sa"
}

resource "ycp_iam_key" "yds_kinesis_proxy_sa_key" {
  lifecycle {
    prevent_destroy = true
  }
  key_id             = ycp_iam_service_account.yds_kinesis_proxy_sa.id
  service_account_id = ycp_iam_service_account.yds_kinesis_proxy_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.yds_kinesis_proxy_sa_public_key
}

resource "ycp_resource_manager_cloud" "ydb_ydbc_cloud" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id        = local.ydb_ydbc_project_id
  name            = "ydb-ydbc"
  organization_id = "yc.organization-manager.yandex"
}

resource "ycp_resource_manager_folder" "ydb_ydbc_folder" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id  = ycp_resource_manager_cloud.ydb_ydbc_cloud.id
  folder_id = local.ydb_ydbc_project_id
  name      = "ydb-ydbc"
}

resource "ycp_iam_service_account" "ydb_controlplane_sa" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = ycp_resource_manager_folder.ydb_ydbc_folder.id
  service_account_id = "yc.ydb.controlplane-sa"
  name               = "ydb-controlplane-sa"
}

resource "ycp_iam_key" "ydb_controlplane_key" {
  lifecycle {
    prevent_destroy = true
  }
  key_id             = ycp_iam_service_account.ydb_controlplane_sa.id
  service_account_id = ycp_iam_service_account.ydb_controlplane_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.ydb_controlplane_sa_public_key
}

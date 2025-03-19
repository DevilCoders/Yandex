locals {
  mdb_control_plane_project_id = "yc.mdb.controlPlane"
  mdb_clusters_project_id = "yc.mdb.clusters"
}

variable "mdb_internal_api_public_key" {
  type = string
}

variable "mdb_backstage_public_key" {
  type = string
}

variable "mdb_worker_public_key" {
  type = string
}

variable "mdb_admin_public_key" {
  type = string
}

variable "mdb_billing_public_key" {
  type = string
}

variable "mdb_e2e_tests_public_key" {
  type = string
}

variable "mdb_reaper_public_key" {
  type = string
}

resource "ycp_resource_manager_cloud" "mdb_control_plane_cloud" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id        = local.mdb_control_plane_project_id
  name            = "mdb-control-plane"
  organization_id = "yc.organization-manager.yandex"
}

resource "ycp_resource_manager_folder" "mdb_control_plane_folder" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id  = ycp_resource_manager_cloud.mdb_control_plane_cloud.id
  folder_id = local.mdb_control_plane_project_id
  name      = "mdb-control-plane"
}

resource "ycp_iam_service_account" "mdb_backstage_sa" {
  folder_id          = ycp_resource_manager_folder.mdb_control_plane_folder.id
  service_account_id = "yc.mdb.backstage"
  name               = "mdb-backstage"
}

resource "ycp_iam_key" "mdb_backstage_key" {
  key_id             = ycp_iam_service_account.mdb_backstage_sa.id
  service_account_id = ycp_iam_service_account.mdb_backstage_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.mdb_backstage_public_key
}

resource "ycp_iam_service_account" "mdb_internal_api_sa" {
  folder_id          = ycp_resource_manager_folder.mdb_control_plane_folder.id
  service_account_id = "yc.mdb.internalApi"
  name               = "mdb-internal-api"
}

resource "ycp_iam_key" "mdb_internal_api_key" {
  key_id             = ycp_iam_service_account.mdb_internal_api_sa.id
  service_account_id = ycp_iam_service_account.mdb_internal_api_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.mdb_internal_api_public_key
}

resource "ycp_iam_service_account" "mdb_worker_sa" {
  folder_id          = ycp_resource_manager_folder.mdb_control_plane_folder.id
  service_account_id = "yc.mdb.worker"
  name               = "mdb-worker"
}

resource "ycp_iam_key" "mdb_worker_key" {
  key_id             = ycp_iam_service_account.mdb_worker_sa.id
  service_account_id = ycp_iam_service_account.mdb_worker_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.mdb_worker_public_key
}

resource "ycp_iam_service_account" "mdb_admin_sa" {
  folder_id          = ycp_resource_manager_folder.mdb_control_plane_folder.id
  service_account_id = "yc.mdb.admin"
  name               = "mdb-admin"
}

resource "ycp_iam_key" "mdb_admin_key" {
  key_id             = ycp_iam_service_account.mdb_admin_sa.id
  service_account_id = ycp_iam_service_account.mdb_admin_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.mdb_admin_public_key
}

resource "ycp_iam_service_account" "mdb_billing_sa" {
  folder_id          = ycp_resource_manager_folder.mdb_control_plane_folder.id
  service_account_id = "yc.mdb.billing"
  name               = "mdb-billing"
}

resource "ycp_iam_key" "mdb_billing_key" {
  key_id             = ycp_iam_service_account.mdb_billing_sa.id
  service_account_id = ycp_iam_service_account.mdb_billing_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.mdb_billing_public_key
}

resource "ycp_iam_service_account" "mdb_e2e_tests_sa" {
  folder_id          = ycp_resource_manager_folder.mdb_control_plane_folder.id
  service_account_id = "yc.mdb.e2eTests"
  name               = "mdb-e2e-tests"
}

resource "ycp_iam_key" "mdb_e2e_tests_key" {
  key_id             = ycp_iam_service_account.mdb_e2e_tests_sa.id
  service_account_id = ycp_iam_service_account.mdb_e2e_tests_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.mdb_e2e_tests_public_key
}

resource "ycp_iam_service_account" "mdb_reaper_sa" {
  folder_id          = ycp_resource_manager_folder.mdb_control_plane_folder.id
  service_account_id = "yc.mdb.reaper"
  name               = "mdb-reaper"
}

resource "ycp_iam_key" "mdb_reaper_key" {
  key_id             = ycp_iam_service_account.mdb_reaper_sa.id
  service_account_id = ycp_iam_service_account.mdb_reaper_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.mdb_reaper_public_key
}

resource "ycp_resource_manager_cloud" "mdb_clusters_cloud" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id        = local.mdb_clusters_project_id
  name            = "mdb-clusters"
  organization_id = "yc.organization-manager.yandex"
}

resource "ycp_resource_manager_folder" "mdb_clusters_folder" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id  = ycp_resource_manager_cloud.mdb_clusters_cloud.id
  folder_id = local.mdb_clusters_project_id
  name      = "mdb-clusters"
}

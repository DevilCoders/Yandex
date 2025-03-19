locals {
  datalens_backend_project_id = "yc.datalens.backend"
  datalens_ui_project_id = "yc.datalens.ui"
}

variable "datalens_back_file_upl_pub_key" {
  type = string
}

variable "datalens_ui_public_key" {
  type = string
}

variable "datalens_ui_notify_public_key" {
  type = string
}

variable "datalens_abs_public_key" {
  type = string
}

resource "ycp_resource_manager_cloud" "datalens_backend_cloud" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id        = local.datalens_backend_project_id
  name            = "datalens-backend"
  organization_id = "yc.organization-manager.yandex"
}

resource "ycp_resource_manager_folder" "datalens_backend_folder" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id  = ycp_resource_manager_cloud.datalens_backend_cloud.id
  folder_id = local.datalens_backend_project_id
  name      = "datalens-backend"
}

resource "ycp_iam_service_account" "datalens_back_file_upl_sa" {
  folder_id          = ycp_resource_manager_folder.datalens_backend_folder.id
  service_account_id = "yc.datalens.back-file-upl-sa"
  name               = "datalens-back-file-upl"
}

resource "ycp_iam_key" "datalens_back_file_upl_key" {
  key_id             = ycp_iam_service_account.datalens_back_file_upl_sa.id
  service_account_id = ycp_iam_service_account.datalens_back_file_upl_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.datalens_back_file_upl_pub_key
}

resource "ycp_resource_manager_cloud" "datalens_ui_cloud" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id        = local.datalens_ui_project_id
  name            = "datalens-ui"
  organization_id = "yc.organization-manager.yandex"
}

resource "ycp_resource_manager_folder" "datalens_ui_folder" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id  = ycp_resource_manager_cloud.datalens_ui_cloud.id
  folder_id = local.datalens_ui_project_id
  name      = "datalens-ui"
}

resource "ycp_iam_service_account" "datalens_ui_sa" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = ycp_resource_manager_folder.datalens_ui_folder.id
  service_account_id = local.datalens_ui_project_id
  name               = "datalens-ui"
}

resource "ycp_iam_key" "datalens_ui_key" {
  lifecycle {
    prevent_destroy = true
  }
  key_id             = ycp_iam_service_account.datalens_ui_sa.id
  service_account_id = ycp_iam_service_account.datalens_ui_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.datalens_ui_public_key
}

resource "ycp_iam_service_account" "datalens_ui_notify_sa" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = ycp_resource_manager_folder.datalens_ui_folder.id
  service_account_id = "yc.datalens.notify"
  name               = "notify"
}

resource "ycp_iam_key" "datalens_ui_notify_key" {
  lifecycle {
    prevent_destroy = true
  }
  key_id             = ycp_iam_service_account.datalens_ui_notify_sa.id
  service_account_id = ycp_iam_service_account.datalens_ui_notify_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.datalens_ui_notify_public_key
}

resource "ycp_iam_service_account" "datalens_abs_sa" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = ycp_resource_manager_folder.datalens_ui_folder.id
  service_account_id = "yc.datalens.abs"
  name               = "datalens-abs"
}

resource "ycp_iam_key" "datalens_abs_key" {
  lifecycle {
    prevent_destroy = true
  }
  key_id             = ycp_iam_service_account.datalens_abs_sa.id
  service_account_id = ycp_iam_service_account.datalens_abs_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.datalens_abs_public_key
}

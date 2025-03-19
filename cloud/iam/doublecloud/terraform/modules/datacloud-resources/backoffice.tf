locals {
  backoffice_ui_project_id = "yc.backoffice.ui"
}

variable "backoffice_ui_sa_public_key" {
  type = string
}

resource "ycp_resource_manager_cloud" "backoffice_ui_cloud" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id        = local.backoffice_ui_project_id
  name            = "backoffice-ui"
  organization_id = "yc.organization-manager.yandex"
}

resource "ycp_resource_manager_folder" "backoffice_ui_folder" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id  = ycp_resource_manager_cloud.backoffice_ui_cloud.id
  folder_id = local.backoffice_ui_project_id
  name      = "backoffice-ui"
}

resource "ycp_iam_service_account" "backoffice_ui_sa" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = ycp_resource_manager_folder.backoffice_ui_folder.id
  service_account_id = local.backoffice_ui_project_id
  name               = "backoffice-ui"
}

resource "ycp_iam_key" "backoffice_ui_sa_public_key" {
  lifecycle {
    prevent_destroy = true
  }
  key_id             = ycp_iam_service_account.backoffice_ui_sa.id
  service_account_id = ycp_iam_service_account.backoffice_ui_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.backoffice_ui_sa_public_key
}

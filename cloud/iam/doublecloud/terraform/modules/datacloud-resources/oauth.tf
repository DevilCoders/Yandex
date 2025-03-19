locals {
  oauth_ssoui_project_id = "yc.oauth.ssoui"
}

variable "oauth_ssoui_public_key" {
  type = string
}

resource "ycp_resource_manager_cloud" "oauth_ssoui_cloud" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id        = local.oauth_ssoui_project_id
  name            = "oauth-ssoui"
  organization_id = "yc.organization-manager.yandex"
}

resource "ycp_resource_manager_folder" "oauth_ssoui_folder" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id  = ycp_resource_manager_cloud.oauth_ssoui_cloud.id
  folder_id = local.oauth_ssoui_project_id
  name      = "oauth-ssoui"
}

resource "ycp_iam_service_account" "oauth_ssoui_sa" {
  folder_id          = ycp_resource_manager_folder.oauth_ssoui_folder.id
  service_account_id = local.oauth_ssoui_project_id
  name               = "oauth-ssoui"
}

resource "ycp_iam_key" "oauth_ssoui_key" {
  key_id             = ycp_iam_service_account.oauth_ssoui_sa.id
  service_account_id = ycp_iam_service_account.oauth_ssoui_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.oauth_ssoui_public_key
}

locals {
  data_transfer_service_project_id = "yc.data-transfer.service"
}

variable "data_transfer_agent_public_key" {
  type = string
}

resource "ycp_resource_manager_cloud" "data_transfer_cloud" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id        = local.data_transfer_service_project_id
  name            = "data-transfer-service"
  organization_id = "yc.organization-manager.yandex"
}

resource "ycp_resource_manager_folder" "data_transfer_folder" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id  = ycp_resource_manager_cloud.data_transfer_cloud.id
  folder_id = local.data_transfer_service_project_id
  name      = "data-transfer-service"
}

resource "ycp_iam_service_account" "data_transfer_agent_sa" {
  folder_id          = ycp_resource_manager_folder.data_transfer_folder.id
  service_account_id = "yc.data-transfer.agent"
  name               = "data-transfer-agent"
}

resource "ycp_iam_key" "data_transfer_agent_key" {
  key_id             = ycp_iam_service_account.data_transfer_agent_sa.id
  service_account_id = ycp_iam_service_account.data_transfer_agent_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.data_transfer_agent_public_key
}

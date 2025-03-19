locals {
  billing_service_project_id = "yc.billing.service"
}

variable "billing_agent_public_key" {
  type = string
}

resource "ycp_resource_manager_cloud" "billing_service_cloud" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id        = local.billing_service_project_id
  name            = "billing-service"
  organization_id = "yc.organization-manager.yandex"
}

resource "ycp_resource_manager_folder" "billing_service_folder" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id  = ycp_resource_manager_cloud.billing_service_cloud.id
  folder_id = local.billing_service_project_id
  name      = "billing-service"
}

resource "ycp_iam_service_account" "billing_agent" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = ycp_resource_manager_folder.billing_service_folder.id
  service_account_id = "yc.billing.agent"
  name               = "yc-billing-agent"
}

resource "ycp_iam_key" "billing_agent_public_key" {
  lifecycle {
    prevent_destroy = true
  }
  key_id             = ycp_iam_service_account.billing_agent.id
  service_account_id = ycp_iam_service_account.billing_agent.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.billing_agent_public_key
}

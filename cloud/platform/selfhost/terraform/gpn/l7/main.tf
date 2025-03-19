terraform {
  required_providers {
    ycp = {
      source  = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = ">= 0.53"
    }
  }

  required_version = ">= 1"
}

provider "ycp" {
  cloud_id  = var.cloud_id
  folder_id = var.folder_id

  ycp_profile = "gpn"
  prod        = false
  ycp_config  = pathexpand("~/.config/ycp/config.yaml")
  # ycp_config               = "../ycp-config.yaml"
  # service_account_key_file = local.gpn_sa_file
}

resource "ycp_iam_service_account" "main" {
  name = "main"
}

resource "ycp_resource_manager_folder_iam_member" "main-sa-editor" {
  folder_id = var.folder_id
  role      = "editor"
  member    = "serviceAccount:${ycp_iam_service_account.main.id}"
}

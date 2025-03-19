terraform {
  required_providers {
    ycp = {
      source  = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      version = ">= 0.60"
    }
  }

  required_version = ">= 1"
}

provider "ycp" {
  prod      = false
  cloud_id  = "aoe0oie417gs45lue0h4"
  folder_id = "aoe4lof1sp0df92r6l8j"
}

# a5d39v77dpdjqsr001m4
resource "ycp_platform_alb_http_router" "cpl-router" {
  lifecycle {
    prevent_destroy = true
    ignore_changes  = [modified_at]
  }
  
  name        = "cpl-router-preprod"
  description = "Root router for control_plane preprod envoy installation"
  folder_id   = "aoe4lof1sp0df92r6l8j"
}

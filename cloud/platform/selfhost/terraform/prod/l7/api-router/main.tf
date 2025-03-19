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
  prod      = true
  cloud_id  = "b1g3clmedscm7uivcn4a"
  folder_id = "b1gvgqhc57450av0d77p"
}

resource "ycp_platform_alb_http_router" "api-router" {
  # id = "ds76jvo202hvcbitb1f0"

  name        = "api-router-prod"
  description = "Root router for api router instances (prod)"
  folder_id   = "b1gl92mdtn95nsd4n4ro"
}

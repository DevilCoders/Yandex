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

locals {
  folder_id = "b1gvgqhc57450av0d77p"
}

resource "ycp_platform_alb_http_router" "dpl01-router" {
  # id           = "ds7bg0fn5m8f3phddumq"
  description    = "Root router for DPL01 router instances (prod)"
  folder_id      = "b1g7fol3sogroski98i6"

  labels         = {}
  name           = "dpl01-router-prod"
}

# DO NOT IMPORT any VH here.
# All the VH are in the ai-gateway folder.

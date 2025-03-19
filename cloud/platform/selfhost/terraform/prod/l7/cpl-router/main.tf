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

resource "ycp_platform_alb_http_router" "cpl-router" {
  # id = ds7v7mmcfkhqsgn034s6
  lifecycle {
    prevent_destroy = true
    ignore_changes  = [modified_at]
  }

  name        = "control-plane-root-router"
  description = "Router for the CPL"
  folder_id   = "b1gvgqhc57450av0d77p"
}

terraform {
  required_providers {
    # Update on Linux:
    # $ curl https://mcdev.s3.mds.yandex.net/terraform-provider-ycp/install.sh | bash
    # Update on Mac:
    # $ brew upgrade terraform-provider-ycp
    ycp = ">= 0.9.1"
  }
}

provider "ycp" {
  prod      = true
  token     = var.yc_token
  cloud_id  = "b1g3clmedscm7uivcn4a"
  folder_id = "b1gvgqhc57450av0d77p"
}

locals {
  name = "jaeger"

  alb_lb = "ds76dumn75n4bvk1otqc"
}

resource "ycp_platform_alb_http_router" "this" {
  name = local.name
  # id = ds78ms0obd2ppvg56vjk
}

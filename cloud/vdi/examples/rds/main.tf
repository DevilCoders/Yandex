terraform {
  required_providers {
    yandex = {
      source = "yandex-cloud/yandex"
    }
  }
}

provider "yandex" {
  cloud_id  = var.cloud_id
  folder_id = var.folder_id
  zone      = var.zone
  token     = var.token
}

resource "random_string" "launch_code" {
  length  = 5
  special = false
  upper   = false
}

output "launch_code" {
  value = random_string.launch_code.result
}

data "yandex_compute_image" "default" {
  family = var.image_family
}

data "template_file" "init" {
  template = file("${path.module}/scripts/init.ps1")
  vars = {
    admin_pass = var.admin_pass
  }
}

locals {
  domain_name = join("", ["rds-", random_string.launch_code.result, ".local"])
}
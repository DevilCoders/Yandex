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

resource "ycp_platform_alb_http_router" "api-router" {
  #id = "a5d48mbh1i5o8dudgs0v"

  name        = "api-router-preprod"
  description = "Root router for api router instances (preprod)"
  folder_id   = "aoekcnbhhbs7f609rhnv"
}

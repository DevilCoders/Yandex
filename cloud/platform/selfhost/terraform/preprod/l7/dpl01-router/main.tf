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


# ycp_platform_alb_http_router.dpl01-router:
resource "ycp_platform_alb_http_router" "dpl01-router" {
  # id = "a5dn4bvq4ltqvm9kguha"

  name        = "dpl01-router-preprod"
  description = "Root router for DPL01 router instances (preprod)"
  folder_id   = "aoestem39nfaq76uepkc"
}

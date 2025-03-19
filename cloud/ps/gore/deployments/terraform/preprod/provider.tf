provider "ycp" {
  ycp_profile = var.ycp_profile
}

provider "yandex" {
  token = var.yc_token
  endpoint = "api.cloud-preprod.yandex.net:443"

  cloud_id = "yc.gore.service-cloud"
  folder_id = "yc.gore.service-folder"
}

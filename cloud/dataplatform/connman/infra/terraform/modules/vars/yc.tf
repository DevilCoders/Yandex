locals {
  yc = {
    "preprod" : local.yc_preprod,
    "prod" : local.yc_prod
  }
  yc_preprod = {
    "endpoint" : "api.cloud-preprod.yandex.net:443"
    "cloud_id" : "aoe8t235vlhnuk4ibvn9"
    "folder_id" : "aoehp0ap8gk6h0ktkevr"
    "zone" : "ru-central1-a"
  }
  yc_prod = {
    "endpoint" : "api.cloud.yandex.net:443"
    "cloud_id" : "b1g37bjrb52ulajsecse"
    "folder_id" : "b1gf09b0lechpmthlccm"
    "zone" : "ru-central1-a"
  }
}

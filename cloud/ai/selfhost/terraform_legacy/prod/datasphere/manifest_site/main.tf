provider "ycp" {
  prod = true
}

module "ai_service_instance_group" {
  source       = "../../../common/datasphere/manifest_site"
  yandex_token = var.yandex_token
  name         = "ai-manifest-site"
  environment  = "prod"
  yc_folder    = "b1g4ujiu1dq5hh9544r6"
  yc_sa_id     = "ajepu21fhq5taau4qb4h"

  yc_instance_group_size = 1

  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting
}

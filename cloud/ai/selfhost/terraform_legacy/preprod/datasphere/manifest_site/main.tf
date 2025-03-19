provider "ycp" {
  prod = true
}

module "ai_service_instance_group" {
  source       = "../../../common/datasphere/manifest_site"
  yandex_token = var.yandex_token
  name         = "ai-manifest-site"
  environment  = "preprod"
  yc_folder    = "b1g19hobememv3hj6qsc"
  yc_sa_id     = "ajec1jspgsvif0dgrao1"

  yc_instance_group_size = 1

  max_unavailable        = var.max_unavailable
  max_creating           = var.max_creating
  max_expansion          = var.max_expansion
  max_deleting           = var.max_deleting
}

provider "ycp" {
    prod = true
}

module "ai_dynconfig_service_instance_group" {
  source          = "../../common/dynconfig"
  yandex_token    = var.yandex_token
  name            = "ai-dynconfig-service"
  environment     = "preprod"
  yc_folder       = "b1gdgmbfkh6f4rlf5puu"
  yc_sa_id        = "ajejueub6j5hpecpo6ig"

  yc_instance_group_size = 1
  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  dynconfig_version = "0.166"
}

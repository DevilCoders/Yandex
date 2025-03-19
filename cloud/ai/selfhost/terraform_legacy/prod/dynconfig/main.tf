provider "ycp" {
  prod = true
}

module "ai_dynconfig_service_instance_group" {
  source          = "../../common/dynconfig"
  yandex_token    = var.yandex_token
  name            = "ai-dynconfig-service"
  environment     = "prod"
  yc_folder       = "b1gb12d4080k0qa9rul2"
  yc_sa_id        = "ajei21097cr8gltuh159"

  yc_instance_group_size = 3
  max_unavailable = var.max_unavailable
  max_creating    = var.max_creating
  max_expansion   = var.max_expansion
  max_deleting    = var.max_deleting

  dynconfig_version = "0.261"
}

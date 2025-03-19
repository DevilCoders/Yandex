provider "yc" {
  service_account_key_file = var.service_account_key_file

  endpoint = module.constants.by_environment.yc_endpoint
  #zone     = var.yc_zone

  folder_id = var.folder_id
}

provider "ycp" {
  service_account_key_file = var.service_account_key_file

  prod = var.environment == "preprod" ? false : true

  folder_id = var.folder_id
}
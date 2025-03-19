module "constants" {
  source      = "../../constants"
  environment = var.environment
}

provider "yandex" {
  service_account_key_file = var.service_account_key_file
  endpoint                 = module.constants.by_environment.yc_endpoint
}
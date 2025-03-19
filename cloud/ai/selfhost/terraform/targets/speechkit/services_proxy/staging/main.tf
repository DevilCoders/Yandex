module "speechkit_services_proxy_staging" {
  source = "../common"

  // Secrets
  yandex_token             = var.yandex_token
  service_account_key_file = var.service_account_key_file

  // Target
  environment        = "staging"

  folder_id          = "b1gd3ibutes0q72uq8uf"
  service_account_id = "ajejsom8kdna3r695d9q"

  instance_group_size = 2
}

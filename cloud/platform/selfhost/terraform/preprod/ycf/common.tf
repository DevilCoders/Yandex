module "ssh-keys" {
  source       = "../../modules/ssh-keys"
  yandex_token = module.oauth.result
  abc_service  = "ycserverles"
}

module "oauth" {
  source = "../../modules/yav-oauth"
}

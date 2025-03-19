module "ssh-keys" {
  source       = "../../modules/ssh-keys"
  yandex_token = var.yandex_token

  abc_service        = "ycl7"
  abc_service_scopes = ["devops", "testing"]
}

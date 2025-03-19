variable "yandex_token" {
  description = "Valid yandex-team token"
}

//variable "abc_service" {
//  description = "Valid yandex-team token"
//}

module "try-get-ssh-keys" {
  source = "../ssh-keys"

  abc_service = "cloud-platform"
  yandex_token = "${var.yandex_token}"
}

output "metadata-ssh-keys" {
  value = "${module.try-get-ssh-keys.ssh-keys}"
}

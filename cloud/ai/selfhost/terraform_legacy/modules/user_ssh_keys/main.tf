//vars
variable "abc_service" {
  description = "ABC Service Slug Name"
  default = "ycai"
}

variable "yandex_token" {
  description = "Yandex Team OAuth Token"
}

// main
data "external" "users_ssh_keys_getter" {
  program = [ "python3",
    "${path.module}/users_ssh_keys_generator.py"]

  query = {
    abc_service = var.abc_service
    yandex_token = var.yandex_token
  }
}

//output
output "ssh_keys" {
  value = data.external.users_ssh_keys_getter.result.ssh_keys
}
output "logins" {
  value = data.external.users_ssh_keys_getter.result.logins
}
output "users" {
  value = data.external.users_ssh_keys_getter.result.users
}

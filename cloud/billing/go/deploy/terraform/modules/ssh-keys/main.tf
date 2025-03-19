//vars
variable "abc_service" {
  description = "ABC Service Slug Name"
  default     = "yandexcloudbilling"
}

variable "abc_service_scopes" {
  description = "ABC Service Scope Slugs (empty means all)"
  default     = []
}

variable "yandex_token" {
  description = "Yandex Team OAuth Token: https://oauth.yandex.ru/authorize?response_type=token&client_id=1a6990aa636648e9b2ef855fa7bec2fb"
  sensitive   = true
}

// main
data "external" "ssh-keys-getter" {
  program = ["python3",
  "${path.module}/ssh_keys_generator.py"]

  query = {
    abc_service        = var.abc_service
    abc_service_scopes = join(",", var.abc_service_scopes)
    yandex_token       = var.yandex_token
  }
}

//output
output "ssh-keys" {
  value = data.external.ssh-keys-getter.result.ssh-keys
}
output "logins" {
  value = data.external.ssh-keys-getter.result.logins
}

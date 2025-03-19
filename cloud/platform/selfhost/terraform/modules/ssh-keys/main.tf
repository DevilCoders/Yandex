//vars
variable "abc_service" {
  description = "ABC Service Slug Name"
  default     = "cloud-platform"
}

variable "abc_service_scopes" {
  description = "ABC Service Scope Slugs (empty means all)"
  default     = []
}

variable "yandex_token" {
  description = "Yandex Team OAuth Token"
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

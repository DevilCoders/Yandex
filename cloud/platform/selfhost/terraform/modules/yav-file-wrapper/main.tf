variable "secret_id" {
  description = "YAV (yav.yandex-team.ru) secret uuid"
}

variable "key_id" {
  description = "key-name inside secret dict"
  default = ""
}

variable "file_path" {
  description = "local file path with secret data to fetch instead of YAV secret"
  default = "_intentionally_empty_file"
}

module "yav-secret-fetch" {
  source    = "../yav"
  secret_id = "${var.secret_id}"
  key_id    = "${var.key_id}"
}

output "value" {
  value  = "${ var.file_path != "_intentionally_empty_file" ? file(var.file_path) : "${module.yav-secret-fetch.value}" }"
}

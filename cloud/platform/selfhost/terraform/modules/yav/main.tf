variable "secret_id" {
  description = "YAV (yav.yandex-team.ru) secret uuid"
}

variable "key_id" {
  description = "key-name inside secret dict"
  default     = ""
}

data "external" "example" {
  program = [
  "${path.module}/code.sh"]

  query = {
    secret_id = var.secret_id
    key_id    = var.key_id
  }
}


output "result" {
  value = data.external.example.result
}

output "value" {
  value = data.external.example.result["value"]
}

output "key" {
  value = data.external.example.result["key"]
}

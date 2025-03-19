# Required
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

variable "service_account_key_file" {
  description = "Path to service account key file"
}

# TODO: Temporary, remove when will be delivered via skm
variable "s3_access_key" {}
variable "s3_secret_key" {}



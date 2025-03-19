# Required
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

variable "service_account_key_file" {
  description = "Path to service account key file"
}

// Input Variables
variable "s3_access_key" {
  type      = string
}

variable "s3_secret_key" {
  type      = string
}

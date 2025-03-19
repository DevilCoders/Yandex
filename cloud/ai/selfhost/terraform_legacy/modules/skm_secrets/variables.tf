variable "encrypted_dek" {
    type = string
}

variable "kms_key_id" {
    type = string
}

variable "skm_path" {
    type = string
    default = "skm"
}

variable "yav_token" {
    type = string
    description = "YAV token (if not specified it will be queries using `ya vault oauth`)"
    default = ""
}

variable "kms_private_endpoint" {
  type = string
  default = ""
}

variable "iam_private_endpoint" {
  type = string
  default = ""
}

variable "cloud_api_endpoint" {
  type = string
  default = "api.cloud.yandex.net:443"
}

variable "yav_secrets" {
    type = map(object({
        path: string
        secret_id: string
    }))
}

variable "file_secrets" {
    type = list(object({
        path: string
        content: string
    }))
    default = []
}

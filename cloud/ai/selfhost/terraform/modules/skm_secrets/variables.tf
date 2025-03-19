variable "encrypted_dek" {
  type = string
}

variable "kms_key_id" {
  type = string
}

variable "skm_path" {
  type    = string
  default = "skm"
}

variable "yav_token" {
  type = string
}

variable "environment" {
  type = string
}

variable "yav_secrets" {
  type = map(object({
    path : string
    secret_id : string
  }))
}

variable "file_secrets" {
  type = list(object({
    path : string
    content : string
  }))
  default = []
}

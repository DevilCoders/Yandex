variable "ycp_profile" {
  type        = string
  description = "Name of the ycp profile"
  default     = "prod"
}

variable "yc_token" {
  type        = string
  description = "OAuth-token used for authentication in Yandex.Cloud. Used in 'yandex' terraform provider. Can be retrieived in yc cli via fed-user 'yc iam create-token'"
}

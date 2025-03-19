variable "ycp_profile" {
  type = string
  description = "Name of the ycp profile"
  default = "preprod"
}

variable "yc_token" {
  type = string
  description = "OAuth-token used for authentication in Yandex.Cloud. Used in 'yandex' terraform provider. Can be retrieived in https://oauth.yandex.ru/authorize?response_type=token&client_id=1a6990aa636648e9b2ef855fa7bec2fb."
}

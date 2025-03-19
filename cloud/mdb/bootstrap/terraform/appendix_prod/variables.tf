variable "cloud_id" {
  type    = string
  default = "b1gt7g3e35pc1h4rvtfn"
}

variable "folder_id" {
  type    = string
  default = "b1gg20u7ddqpr2fbf3mb"
}

variable "region_id" {
  type    = string
  default = "ru-central1"
}

variable "yc_token" {
  type        = string
  description = "Obtain token here https://oauth.yandex.ru/authorize?response_type=token&client_id=1a6990aa636648e9b2ef855fa7bec2fb"
  default     = null
}

variable "service_account_key_file" {
  type    = string
  default = null
}


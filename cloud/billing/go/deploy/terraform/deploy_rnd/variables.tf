// Provider Specific

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"
  default     = "b1gjvj0ghj6u2crd72gk"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisoned resources"
  default     = "ru-central1-c"
}

variable "yc_endpoint" {
  description = "Yandex Cloud API endpoint"
  default     = "api.cloud.yandex.net:443"
}

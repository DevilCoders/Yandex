// SSH keys Specific

variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

// Provider Specific

variable "yc_token" {
  description = "Yandex Cloud security OAuth token"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"
  default     = "aoe8qo1krj6vc24sh2dh" // compute
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisoned resources"
  default     = "ru-central1-c"
}

variable "yc_endpoint" {
  description = "Yandex Cloud API endpoint"
  default     = "api.cloud-preprod.yandex.net:443"
}

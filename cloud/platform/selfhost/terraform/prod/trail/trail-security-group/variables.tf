variable "yc_endpoint" {
  description = "Yandex Cloud API endpoint"
  default     = "api.cloud.yandex.net:443"
}

variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

variable "yc_token" {
  description = "IAM token"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"
  // yc-cloud-trail/default
  default = "b1gtrvd4ntcgpob8ga7r"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisioned resources"
  default     = "ru-central1"
}

variable "network_id" {
  description = "Network id"
  default = "enpsm2k46k6n43vrk6fg"
}
